/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_databuffer.h>
#include <et/rendering/vulkan/vulkan_indexbuffer.h>
#include <et/rendering/vulkan/vulkan_vertexbuffer.h>
#include <et/rendering/vulkan/vulkan_pipelinestate.h>
#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan_renderpass.h>
#include <et/rendering/vulkan/vulkan_texture.h>
#include <et/rendering/vulkan/vulkan_sampler.h>
#include <et/rendering/vulkan/vulkan_renderer.h>
#include <et/rendering/vulkan/vulkan.h>
#include <et/app/application.h>

#define VULKAN_ENABLE_VALIDATION 0

namespace et
{
class VulkanRendererPrivate : public VulkanState
{
public:
	VulkanState& vulkan() 
		{ return *this; }

	PipelineStateCache pipelineCache;
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

VkResult vkEnumerateInstanceLayerPropertiesWrapper(int, uint32_t* count, VkLayerProperties* props)
{
	return vkEnumerateInstanceLayerProperties(count, props);
}

VkBool32 vulkanDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj,
	size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		log::error("%s : %s", layerPrefix, msg);
		debug::debugBreak();
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		log::warning("%s : %s", layerPrefix, msg);
	}
	else
	{
		log::info("%s : %s", layerPrefix, msg);
	}
	return VK_TRUE;
}

void VulkanRenderer::init(const RenderContextParameters& params)
{
	std::vector<const char*> enabledExtensions = 
	{ 
		VK_KHR_SURFACE_EXTENSION_NAME, 
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME, 
#	if (VULKAN_ENABLE_VALIDATION)
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#	endif
	};

	Vector<const char*> validationLayers;

#if (VULKAN_ENABLE_VALIDATION)
	auto layerProps = enumerateVulkanObjects<VkLayerProperties>(0, vkEnumerateInstanceLayerPropertiesWrapper);
	validationLayers.reserve(4);
	for (const auto& layerProp : layerProps)
	{
		if (strstr(layerProp.layerName, "validation"))
		{
			validationLayers.push_back(layerProp.layerName);
		}
	}
#endif

	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = application().identifier().applicationName.c_str();
	appInfo.pEngineName = "et-engine";
	appInfo.apiVersion = VK_API_VERSION_1_0;
	VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	VULKAN_CALL(vkCreateInstance(&instanceCreateInfo, nullptr, &_private->instance));

#if (VULKAN_ENABLE_VALIDATION)
	PFN_vkCreateDebugReportCallbackEXT createDebugCb = 
		reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(_private->instance, "vkCreateDebugReportCallbackEXT"));

	if (createDebugCb)
	{
		VkDebugReportCallbackCreateInfoEXT debugInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
		debugInfo.flags = VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
		debugInfo.pfnCallback = reinterpret_cast<PFN_vkDebugReportCallbackEXT>(vulkanDebugCallback);
		VULKAN_CALL(createDebugCb(_private->instance, &debugInfo, nullptr, &_private->debugCallback));
	}
#endif

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
	VULKAN_CALL(vkCreateSemaphore(_private->device, &semaphoreInfo, nullptr, &_private->semaphores.renderComplete));
	VULKAN_CALL(vkCreateSemaphore(_private->device, &semaphoreInfo, nullptr, &_private->semaphores.imageAvailable));

	HWND window = reinterpret_cast<HWND>(application().context().objects[0]);
	_private->swapchain.init(_private->vulkan(), params, window);

	VkCommandBufferAllocateInfo serviceBufferInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	serviceBufferInfo.commandPool = _private->commandPool;
	serviceBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	serviceBufferInfo.commandBufferCount = 1;
	VULKAN_CALL(vkAllocateCommandBuffers(_private->device, &serviceBufferInfo, &_private->serviceCommandBuffer));

	_private->swapchain.create(_private->vulkan());
	
	initInternalStructures();
}

void VulkanRenderer::shutdown()
{
	shutdownInternalStructures();

	// TODO : kill all
}

void VulkanRenderer::begin()
{
	_private->swapchain.acquireNextImage(_private->vulkan());
}

void VulkanRenderer::present()
{
	_private->swapchain.present(_private->vulkan());
}

DataBuffer::Pointer VulkanRenderer::createDataBuffer(const std::string& name, uint32_t size)
{
	return VulkanDataBuffer::Pointer::create(_private->vulkan(), size);
}

DataBuffer::Pointer VulkanRenderer::createDataBuffer(const std::string& name, const BinaryDataStorage& data)
{
	return VulkanDataBuffer::Pointer::create(_private->vulkan(), data);
}

VertexBuffer::Pointer VulkanRenderer::createVertexBuffer(const std::string& name, VertexStorage::Pointer vs, BufferDrawType dt)
{
	return VulkanVertexBuffer::Pointer::create(_private->vulkan(), vs->declaration(), vs->data(), dt, name);
}

IndexBuffer::Pointer VulkanRenderer::createIndexBuffer(const std::string& name, IndexArray::Pointer ia, BufferDrawType dt)
{
	return VulkanIndexBuffer::Pointer::create(_private->vulkan(), ia, dt, name);
}

Texture::Pointer VulkanRenderer::createTexture(TextureDescription::Pointer desc)
{
	return VulkanTexture::Pointer::create(_private->vulkan(), desc);
}

Sampler::Pointer VulkanRenderer::createSampler(const Sampler::Description& desc)
{
	return VulkanSampler::Pointer::create(_private->vulkan(), desc);
}

Program::Pointer VulkanRenderer::createProgram(const std::string& source)
{
	VulkanProgram::Pointer program = VulkanProgram::Pointer::create(_private->vulkan());
	program->build(source);
	return program;
}

PipelineState::Pointer VulkanRenderer::acquirePipelineState(RenderPass::Pointer pass, Material::Pointer mat, VertexStream::Pointer vs)
{
	auto ps = _private->pipelineCache.find(vs->vertexBuffer()->declaration(), mat->program(), mat->depthState(),
		mat->blendState(), mat->cullMode(), TextureFormat::RGBA8, vs->indexBuffer()->primitiveType());

	if (ps.invalid())
	{
		ps = VulkanPipelineState::Pointer::create(this, _private->vulkan());
		ps->setRenderPass(pass);
		ps->setBlendState(mat->blendState());
		ps->setCullMode(mat->cullMode());
		ps->setDepthState(mat->depthState());
		ps->setInputLayout(vs->vertexBuffer()->declaration());
		ps->setProgram(mat->program());
		ps->setPrimitiveType(vs->indexBuffer()->primitiveType());
		ps->build();
		_private->pipelineCache.addToCache(ps);
	}

	return ps;
}

RenderPass::Pointer VulkanRenderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	return VulkanRenderPass::Pointer::create(this, _private->vulkan(), info);
}

void VulkanRenderer::submitRenderPass(RenderPass::Pointer pass)
{
	VulkanRenderPass::Pointer vkPass = pass;
	vkPass->submit();
}

}
