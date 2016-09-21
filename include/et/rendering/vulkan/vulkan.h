/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/rendering.h>

#if (ET_PLATFORM_WIN)
#	define VK_USE_PLATFORM_WIN32_KHR
#	include <et/rendering/vulkan/vulkan/vulkan.h>
#else
#	error Not implemented for this platform
#endif

namespace et
{

struct VulkanState;
struct RenderContextParameters;

struct VulkanSwapchain
{
	void init(VulkanState& vulkan, const RenderContextParameters& params, HWND window);
	void create(VulkanState& vulkan);

	void acquireNextImage(VulkanState& vulkan);
	void present(VulkanState& vulkan);

	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapchain = nullptr;
	
	VkExtent2D extent { };
	VkSurfaceFormatKHR surfaceFormat { };

	struct RenderTarget
	{
		VkImage image = nullptr;
		VkImageView imageView = nullptr;
	};
	Vector<RenderTarget> images;
	uint32_t currentImageIndex = static_cast<uint32_t>(-1);

	VkImage currentImage() const 
		{ return images.at(currentImageIndex).image; }
};

struct VulkanState
{
	VkInstance instance = nullptr;
	VkDebugReportCallbackEXT debugCallback = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkDevice device = nullptr;
	VkCommandPool commandPool = nullptr;
	VkQueue queue = nullptr;
	VkPipelineCache pipelineCache = nullptr;

	struct Semaphores
	{
		VkSemaphore imageAvailable = nullptr;
		VkSemaphore renderComplete = nullptr;
	} semaphores;

	VkCommandBuffer setupCommandBuffer = nullptr;
	
	Vector<VkQueueFamilyProperties> queueProperties;
	uint32_t graphicsQueueIndex = static_cast<uint32_t>(-1);
	uint32_t presentQueueIndex = static_cast<uint32_t>(-1);

	VulkanSwapchain swapchain;
};

struct VulkanShaderModules
{
	VkShaderModule fragment = nullptr;
	VkShaderModule vertex = nullptr;
	VkPipelineShaderStageCreateInfo stageCreateInfo[2]
	{ 
		{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO }, 
		{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO } 
	};
};

struct VulkanNativeRenderPass
{
	VkFramebuffer framebuffer = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	VkRenderPass renderPass = nullptr;
	VkViewport viewport { };
	VkRect2D scissor { };
};

struct VulkanNativePipeline
{
	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	VkPipelineLayout layout = nullptr;
	VkPipeline pipeline = nullptr;
};

namespace vulkan
{

VkCompareOp depthCompareOperation(CompareFunction);
VkFormat dataTypeValue(DataType);
VkPrimitiveTopology primitiveTopology(PrimitiveType);
VkCullModeFlags cullModeFlags(CullMode);
VkIndexType indexBufferFormat(IndexArrayFormat);

}

const char* vulkanResultToString(VkResult result);

#define VULKAN_CALL(expr) { \
	auto result = (expr); \
	if (result != VkResult::VK_SUCCESS) \
	{ \
		et::log::error("Vulkan call failed: %s\nat %s [%d]\nresult = %s", \
			(#expr), __FILE__, __LINE__, vulkanResultToString(result)); \
		et::debug::debugBreak(); \
	}}

template <class EnumeratedClass, class Holder, typename Function, typename ReturnClass>
struct VulkanObjectsEnumerator;

template <class EnumeratedClass, class Holder, typename Function>
struct VulkanObjectsEnumerator<EnumeratedClass, Holder, Function, VkResult>
{
	Vector<EnumeratedClass> operator ()(const Holder& holder, Function callable)
	{
		uint32_t count = 0;
		Vector<EnumeratedClass> objects;
		VULKAN_CALL(callable(holder, &count, nullptr));
		if (count > 0)
		{
			objects.resize(count);
			VULKAN_CALL(callable(holder, &count, objects.data()));
		}
		return objects;
	}
};

template <class EnumeratedClass, class Holder, typename Function>
struct VulkanObjectsEnumerator<EnumeratedClass, Holder, Function, void>
{
	Vector<EnumeratedClass> operator ()(const Holder& holder, Function callable)
	{
		uint32_t count = 0;
		Vector<EnumeratedClass> objects;
		callable(holder, &count, nullptr);
		if (count > 0)
		{
			objects.resize(count);
			callable(holder, &count, objects.data());
		}
		return objects;
	}
};

template <class EnumeratedClass, class Holder, typename Function>
Vector<EnumeratedClass> enumerateVulkanObjects(const Holder& holder, Function callable)
{
	using ReturnClass = decltype(callable(holder, nullptr, nullptr));
	VulkanObjectsEnumerator<EnumeratedClass, Holder, Function, ReturnClass> enumerator;
	return enumerator(holder, callable);
}

uint32_t getVulkanMemoryType(VulkanState& vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties);

class VulkanNativeBuffer
{
public:
	VulkanNativeBuffer(VulkanState& vulkan, uint32_t size, uint32_t usage);
	~VulkanNativeBuffer();

	void* map(uint32_t offset, uint32_t size);
	void unmap();

	VkBuffer buffer() const 
		{ return _buffer; }

	bool mapped() const 
		{ return _mapped; }

private:
	VulkanState& _vulkan;
	VkBuffer _buffer = nullptr;
	VkDeviceMemory _memory = nullptr;
	std::atomic_bool _mapped{false};
};

}