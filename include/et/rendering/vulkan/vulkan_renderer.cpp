/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_buffer.h>
#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan_texture.h>
#include <et/rendering/vulkan/vulkan_sampler.h>
#include <et/rendering/vulkan/vulkan_renderpass.h>
#include <et/rendering/vulkan/vulkan_textureset.h>
#include <et/rendering/vulkan/vulkan_pipelinestate.h>
#include <et/rendering/vulkan/vulkan_renderer.h>
#include <et/rendering/vulkan/vulkan.h>
#include <et/rendering/vulkan/glslang/vulkan_glslang.h>
#include <et/app/application.h>

#if (ET_DEBUG)
#	define VULKAN_ENABLE_VALIDATION 1
#else
#	define VULKAN_ENABLE_VALIDATION 0
#endif

#define VULKAN_COMBINED_SUBMIT 1

namespace et
{
class VulkanRendererPrivate : public VulkanState
{
public:
	VulkanState& vulkan() {
		return *this;
	}

	PipelineStateCache pipelineCache;
	std::array<Vector<VulkanRenderPass::Pointer>, RendererFrameCount> passes;
};

VulkanRenderer::VulkanRenderer(RenderContext* rc)
	: RenderInterface(rc)
{
	ET_PIMPL_INIT(VulkanRenderer);
	Camera::renderingOriginTransform = -1.0f;
}

VulkanRenderer::~VulkanRenderer()
{
	ET_PIMPL_FINALIZE(VulkanRenderer);
}

VkResult vkEnumerateInstanceLayerPropertiesWrapper(int, uint32_t* count, VkLayerProperties* props)
{
	return vkEnumerateInstanceLayerProperties(count, props);
}

VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj,
	size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
	if (obj == static_cast<uint64_t>(-1))
		obj = 0;

	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		log::error("%s [%llx] : %s", layerPrefix, obj, msg);
		debug::debugBreak();
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		log::warning("%s [%llx] : %s", layerPrefix, obj, msg);
	}
	else
	{
		log::info("%s [%llx] : %s", layerPrefix, obj, msg);
	}
	return VK_FALSE;
}

void VulkanRenderer::init(const RenderContextParameters& params)
{
	initGlslangResources();

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
			validationLayers.emplace_back(layerProp.layerName);
			log::info("Vulkan validation layer used: %s (%s)", layerProp.layerName, layerProp.description);
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
	vkGetPhysicalDeviceProperties(_private->physicalDevice, &_private->physicalDeviceProperties);

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

	HWND mainWindow = reinterpret_cast<HWND>(application().context().objects[0]);
	_private->swapchain.init(_private->vulkan(), params, mainWindow);

	VkCommandBufferAllocateInfo serviceBufferInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	serviceBufferInfo.commandPool = _private->commandPool;
	serviceBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	serviceBufferInfo.commandBufferCount = 1;
	VULKAN_CALL(vkAllocateCommandBuffers(_private->device, &serviceBufferInfo, &_private->serviceCommandBuffer));

	uint32_t defaultPoolSize = 1024;
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, defaultPoolSize },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, defaultPoolSize },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, defaultPoolSize },
		{ VK_DESCRIPTOR_TYPE_SAMPLER, defaultPoolSize },
	};

	VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = defaultPoolSize;
	poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
	poolInfo.pPoolSizes = poolSizes;
	VULKAN_CALL(vkCreateDescriptorPool(_private->device, &poolInfo, nullptr, &_private->descriptorPool));

	RECT clientRect = {};
	GetClientRect(mainWindow, &clientRect);
	resize(vec2i(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top));

	initInternalStructures();
}

void VulkanRenderer::shutdown()
{
	VULKAN_CALL(vkDeviceWaitIdle(_private->device));

	for (auto& pass : _private->passes)
		pass.clear();
}

void VulkanRenderer::destroy()
{
	_private->pipelineCache.clear();
	shutdownInternalStructures();

	vkDestroyDescriptorPool(_private->device, _private->descriptorPool, nullptr);
	// TODO : clean up Vulkan

	cleanupGlslangResources();
}

void VulkanRenderer::resize(const vec2i& sz)
{
	_private->swapchain.createSizeDependentResources(_private->vulkan(), sz);
}

Buffer::Pointer VulkanRenderer::createBuffer(const std::string&, const Buffer::Description& desc)
{
	return VulkanBuffer::Pointer::create(_private->vulkan(), desc);
}

Texture::Pointer VulkanRenderer::createTexture(const TextureDescription::Pointer& desc)
{
	return VulkanTexture::Pointer::create(_private->vulkan(), desc.reference(), desc->data);
}

TextureSet::Pointer VulkanRenderer::createTextureSet(const TextureSet::Description& desc)
{
	return VulkanTextureSet::Pointer::create(this, _private->vulkan(), desc);
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

PipelineState::Pointer VulkanRenderer::acquirePipelineState(const RenderPass::Pointer& pass, const Material::Pointer& mat,
	const VertexStream::Pointer& vs)
{
	const std::string& cls = pass->info().name;
	const Material::Configuration& config = mat->configuration(cls);

	VulkanPipelineState::Pointer ps = _private->pipelineCache.find(pass->identifier(), vs->vertexDeclaration(), config.program,
		config.depthState, config.blendState, config.cullMode, vs->primitiveType());

	if (ps.invalid())
	{
		ps = VulkanPipelineState::Pointer::create(this, _private->vulkan());
		ps->setPrimitiveType(vs->primitiveType());
		ps->setInputLayout(vs->vertexDeclaration());
		ps->setDepthState(config.depthState);
		ps->setBlendState(config.blendState);
		ps->setCullMode(config.cullMode);
		ps->setProgram(config.program);
		ps->build(pass);

		_private->pipelineCache.addToCache(pass, ps);
	}

	return ps;
}

RenderPass::Pointer VulkanRenderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	return VulkanRenderPass::Pointer::create(this, _private->vulkan(), info);
}

void VulkanRenderer::submitRenderPass(RenderPass::Pointer inPass)
{
	_private->passes[_private->swapchain.frameIndex].emplace_back(inPass);
}

void VulkanRenderer::begin()
{
	_private->swapchain.acquireNextImage(_private->vulkan());
	_private->passes[_private->swapchain.frameIndex].clear();
}

void VulkanRenderer::present()
{
	static VkPipelineStageFlags firstWaitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	static VkPipelineStageFlags lastWaitStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	Vector<VulkanRenderPass::Pointer>& passes = _private->passes[_private->swapchain.frameIndex];
	std::stable_sort(passes.begin(), passes.end(), [](const VulkanRenderPass::Pointer& l, const VulkanRenderPass::Pointer& r) {
		return l->info().priority > r->info().priority;
	});

	sharedConstantBuffer().flush();

	for (VulkanRenderPass::Pointer pass : passes)
		pass->recordCommandBuffer();

	size_t maxQueueSize = 2 * (1 + std::min(32ull, sqr(_private->passes.size())));
	Vector<VkSubmitInfo> allSubmits;
	Vector<VkSemaphore> waitSemaphores;
	Vector<VkSemaphore> signalSemaphores;
	Vector<VkPipelineStageFlags> waitStages;
	Vector<VkCommandBuffer> commandBuffers;
	allSubmits.reserve(maxQueueSize);
	commandBuffers.reserve(maxQueueSize);
	waitStages.reserve(maxQueueSize);
	waitSemaphores.reserve(maxQueueSize);
	signalSemaphores.reserve(maxQueueSize);

	{
		allSubmits.emplace_back();
		VkSubmitInfo& submitInfo = allSubmits.back();
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_private->swapchain.currentFrame().barrierFromPresent;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &_private->swapchain.currentFrame().imageAcquired;
		submitInfo.pWaitDstStageMask = &firstWaitStage;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &_private->swapchain.currentFrame().semaphoreFromPresent;
	}

	uint32_t commandBuffersBegin = 0;
	uint32_t waitBegin = 0;
	uint32_t signalBegin = 0;
	uint32_t submittedCommandBuffers = 0;
	uint32_t submittedWait = 0;
	uint32_t submittedSignal = 0;
	for (auto i = passes.begin(), e = passes.end(); i != e; ++i)
	{
		uint32_t commandBuffersEnd = commandBuffersBegin;
		uint32_t waitEnd = waitBegin;
		uint32_t signalEnd = signalBegin;

		if (i == passes.begin())
		{
			waitSemaphores.emplace_back(_private->swapchain.currentFrame().semaphoreFromPresent);
			waitStages.emplace_back(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
			++waitEnd;
		}

		for (auto i = signalSemaphores.begin() + signalBegin - submittedSignal, e = signalSemaphores.end(); i != e; ++i)
		{
			waitSemaphores.emplace_back(*i);
			waitStages.emplace_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			++waitEnd;
		}

		const VulkanRenderPass::Pointer& passI = *i;
		signalSemaphores.emplace_back(passI->nativeRenderPass().semaphore);
		commandBuffers.emplace_back(passI->nativeRenderPass().commandBuffers[_private->swapchain.frameIndex]);
		++commandBuffersEnd;
		++signalEnd;

		for (auto j = i + 1; (j != e) && ((*j)->info().priority == (*i)->info().priority); ++j, ++i)
		{
			const VulkanRenderPass::Pointer& passJ = *j;
			commandBuffers.emplace_back(passJ->nativeRenderPass().commandBuffers[_private->swapchain.frameIndex]);
			signalSemaphores.emplace_back(passJ->nativeRenderPass().semaphore);
			++commandBuffersEnd;
			++signalEnd;
		}

		if (i + 1 == e)
		{
			signalSemaphores.emplace_back(_private->swapchain.currentFrame().semaphoreToPresent);
			signalBegin = static_cast<uint32_t>(signalSemaphores.size()) - 1;
			signalEnd = signalBegin + 1;
		}

		submittedCommandBuffers = commandBuffersEnd - commandBuffersBegin;
		submittedSignal = signalEnd - signalBegin;
		submittedWait = waitEnd - waitBegin;

		allSubmits.emplace_back();
		VkSubmitInfo& submitInfo = allSubmits.back();
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.pCommandBuffers = commandBuffers.data() + commandBuffersBegin;
		submitInfo.commandBufferCount = submittedCommandBuffers;
		ET_ASSERT(submitInfo.commandBufferCount > 0);

		submitInfo.pSignalSemaphores = signalSemaphores.data() + signalBegin;
		submitInfo.signalSemaphoreCount = submittedSignal;
		ET_ASSERT(submitInfo.signalSemaphoreCount > 0);

		submitInfo.pWaitDstStageMask = waitStages.data() + waitBegin;
		submitInfo.pWaitSemaphores = waitSemaphores.data() + waitBegin;
		submitInfo.waitSemaphoreCount = submittedWait;
		ET_ASSERT(submitInfo.waitSemaphoreCount > 0);

		commandBuffersBegin = commandBuffersEnd;
		signalBegin = signalEnd;
		waitBegin = waitEnd;
	}

	{
		allSubmits.emplace_back();
		VkSubmitInfo& submitInfo = allSubmits.back();
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_private->swapchain.currentFrame().barrierToPresent;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &_private->swapchain.currentFrame().semaphoreToPresent;
		submitInfo.pWaitDstStageMask = &lastWaitStage;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &_private->swapchain.currentFrame().submitCompleted;
	}

#if (VULKAN_COMBINED_SUBMIT)
	VULKAN_CALL(vkQueueSubmit(_private->queue, static_cast<uint32_t>(allSubmits.size()), allSubmits.data(), 
		_private->swapchain.currentFrame().imageFence));
	allSubmits.clear();
#endif
	
	_private->swapchain.present(_private->vulkan());
}


}
