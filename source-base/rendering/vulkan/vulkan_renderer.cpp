/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_indexbuffer.h>
#include <et/rendering/vulkan/vulkan_pipelinestate.h>
#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan_renderpass.h>
#include <et/rendering/vulkan/vulkan_texture.h>
#include <et/rendering/vulkan/vulkan_vertexbuffer.h>
#include <et/rendering/vulkan/vulkan_renderer.h>
#include <et/rendering/vulkan/vulkan.h>
#include <et/app/application.h>

namespace et
{
class VulkanRendererPrivate : public VulkanState
{
public:
	VulkanState& vulkan() 
		{ return *this; }
};

VulkanRenderer::VulkanRenderer(RenderContext* rc) 
	: RenderInterface(rc)
{
	ET_PIMPL_INIT(VulkanRenderer)
}

VulkanRenderer::~VulkanRenderer()
{
	ET_PIMPL_FINALIZE(VulkanRenderer)
}

void VulkanRenderer::init(const RenderContextParameters& params)
{
	std::vector<const char*> enabledExtensions = 
	{ 
		VK_KHR_SURFACE_EXTENSION_NAME, 
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME, 
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	};

	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = application().identifier().applicationName.c_str();
	appInfo.pEngineName = "et-engine";
	appInfo.apiVersion = VK_API_VERSION_1_0;
	VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	VULKAN_CALL(vkCreateInstance(&instanceCreateInfo, nullptr, &_private->instance));

	auto physicalDevices = enumerateVulkanObjects<VkPhysicalDevice>(_private->instance, vkEnumeratePhysicalDevices);
	ET_ASSERT(!physicalDevices.empty());

	_private->physicalDevice = physicalDevices.front();

	_private->queueProperties = enumerateVulkanObjects<VkQueueFamilyProperties>(_private->physicalDevice, vkGetPhysicalDeviceQueueFamilyProperties);
	ET_ASSERT(!_private->queueProperties.empty());

	_private->graphicsQueueIndex = static_cast<uint32_t>(-1);
	for (size_t i = 0, e = _private->queueProperties.size(); i < e; ++i)
	{
		if (_private->queueProperties.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			_private->graphicsQueueIndex = static_cast<uint32_t>(i);
			break;
		}
	}
	ET_ASSERT(_private->graphicsQueueIndex != static_cast<uint32_t>(-1));

	float queuePriorities[] = { 0.0f };
	VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(_private->graphicsQueueIndex);
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = queuePriorities;

	Vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	VULKAN_CALL(vkCreateDevice(_private->physicalDevice, &deviceCreateInfo, nullptr, &_private->device));

	VkCommandPoolCreateInfo cmdPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	cmdPoolCreateInfo.queueFamilyIndex = _private->graphicsQueueIndex;
	cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VULKAN_CALL(vkCreateCommandPool(_private->device, &cmdPoolCreateInfo, nullptr, &_private->commandPool));

	vkGetDeviceQueue(_private->device, _private->graphicsQueueIndex, 0, &_private->queue);

	VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VULKAN_CALL(vkCreateSemaphore(_private->device, &semaphoreInfo, nullptr, &_private->semaphores.render));
	VULKAN_CALL(vkCreateSemaphore(_private->device, &semaphoreInfo, nullptr, &_private->semaphores.present));

	_private->submitInfo.signalSemaphoreCount = 1;
	_private->submitInfo.pSignalSemaphores = &_private->semaphores.render;
	_private->submitInfo.waitSemaphoreCount = 1;
	_private->submitInfo.pWaitSemaphores = &_private->semaphores.present;

	HWND window = reinterpret_cast<HWND>(application().context().objects[0]);
	_private->swapchain.init(_private->vulkan(), params, window);

	VkCommandBufferAllocateInfo setupCBInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	setupCBInfo.commandPool = _private->commandPool;
	setupCBInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	setupCBInfo.commandBufferCount = 1;
	VULKAN_CALL(vkAllocateCommandBuffers(_private->device, &setupCBInfo, &_private->setupCommandBuffer));

	// VkCommandBufferBeginInfo setupBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; 
	// vkBeginCommandBuffer(_private->setupCommandBuffer, &setupBeginInfo);

	_private->swapchain.create(_private->vulkan());
}

void VulkanRenderer::shutdown()
{
	// TODO : kill all
}

void VulkanRenderer::begin()
{
	_private->swapchain.acquireNextImage(_private->vulkan());
}

void VulkanRenderer::present()
{
	VkPresentInfoKHR info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	info.swapchainCount = 1;
	info.pSwapchains = &_private->swapchain.swapchain;
	info.pImageIndices = &_private->swapchain.currentImageIndex;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &_private->semaphores.render;
	VULKAN_CALL(vkQueuePresentKHR(_private->queue, &info));
}

VertexBuffer::Pointer VulkanRenderer::createVertexBuffer(const std::string& name, VertexStorage::Pointer vs, BufferDrawType dt)
{
	return VulkanVertexBuffer::Pointer::create(vs->declaration(), dt, name);
}

IndexBuffer::Pointer VulkanRenderer::createIndexBuffer(const std::string& name, IndexArray::Pointer ia, BufferDrawType dt)
{
	return VulkanIndexBuffer::Pointer::create(ia, dt, name);
}

VertexArrayObject::Pointer VulkanRenderer::createVertexArrayObject(const std::string& name)
{
	return VertexArrayObject::Pointer::create(name);
}

Texture::Pointer VulkanRenderer::createTexture(TextureDescription::Pointer desc)
{
	return VulkanTexture::Pointer::create(desc);
}

Program::Pointer VulkanRenderer::createProgram(const std::string& vs, const std::string& fs,
	const StringList& defines, const std::string& baseFolder)
{
	return VulkanProgram::Pointer::create();
}

PipelineState::Pointer VulkanRenderer::createPipelineState(RenderPass::Pointer, Material::Pointer, VertexArrayObject::Pointer)
{
	return VulkanPipelineState::Pointer::create();
}

RenderPass::Pointer VulkanRenderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	return VulkanRenderPass::Pointer::create(info);
}

void VulkanRenderer::submitRenderPass(RenderPass::Pointer)
{

}

void VulkanRenderer::drawIndexedPrimitive(PrimitiveType, IndexArrayFormat, uint32_t first, uint32_t count)
{

}

}
