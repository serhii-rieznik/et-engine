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
	void createSizeDependentResources(VulkanState& vulkan, const vec2i&);

	void acquireNextImage(VulkanState& vulkan);
	void present(VulkanState& vulkan);

	bool createDepthImage(VulkanState& vulkan, VkImage& image, VkDeviceMemory& memory, VkCommandBuffer cmdBuffer);
	VkImageView createImageView(VulkanState&, VkImage, VkImageAspectFlags, VkFormat);

	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapchain = nullptr;
	
	VkExtent2D extent { };
	VkSurfaceFormatKHR surfaceFormat { };
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	struct RenderTarget
	{
		VkImage color = nullptr;
		VkImageView colorView = nullptr;
		VkImage depth = nullptr;
		VkImageView depthView = nullptr;
		VkDeviceMemory depthMemory = nullptr;
	};
	Vector<RenderTarget> images;
	uint32_t currentImageIndex = static_cast<uint32_t>(-1);

	const RenderTarget& currentRenderTarget() const
		{ return images.at(currentImageIndex); }

	VkImage currentColorImage() const 
		{ return images.at(currentImageIndex).color; }

	VkImage currentDepthImage() const 
		{ return images.at(currentImageIndex).depth; }
};

struct VulkanState
{
	VkInstance instance = nullptr;
	VkDebugReportCallbackEXT debugCallback = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkDevice device = nullptr;
	VkCommandPool commandPool = nullptr;
	VkDescriptorPool descriptprPool = nullptr;
	VkQueue queue = nullptr;
	VkFence submitFence = nullptr;
	VkPipelineCache pipelineCache = nullptr;
	VkCommandBuffer serviceCommandBuffer = nullptr;

	struct Semaphores
	{
		VkSemaphore imageAvailable = nullptr;
		VkSemaphore renderComplete = nullptr;
	} semaphores;
	
	Vector<VkQueueFamilyProperties> queueProperties;
	uint32_t graphicsQueueIndex = static_cast<uint32_t>(-1);
	uint32_t presentQueueIndex = static_cast<uint32_t>(-1);

	VulkanSwapchain swapchain;

	using ServiceCommands = std::function<void(VkCommandBuffer)>;
	void executeServiceCommands(ServiceCommands);
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
	VkRenderPass renderPass = nullptr;
	VkCommandBuffer commandBuffer { };

	VkDescriptorSetLayout dynamicDescriptorSetLayout = nullptr;
	VkDescriptorSet dynamicDescriptorSet = nullptr;

	VkSemaphore semaphore = nullptr;
	
	VkViewport viewport { };
	VkRect2D scissor { };
};

enum DescriptorSetClass : uint32_t
{
	Buffers,
	Textures,
	Count,
	
	DynamicDescriptorsCount = 2
};

struct VulkanNativePipeline
{
	VkPipeline pipeline = nullptr;
	VkPipelineLayout layout = nullptr;
	VkDescriptorSetLayout texturesLayout = nullptr;
};

struct VulkanNativeTexture
{
	VkImage image = nullptr;
	VkImageView imageView = nullptr;
	VkDeviceMemory memory = nullptr;
	VkMemoryRequirements memoryRequirements { };
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

struct VulkanNativeSampler
{
	VkSampler sampler = nullptr;
};

struct VulkanNativeTextureSet
{
	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	VkDescriptorSet descriptorSet = nullptr;
};

struct VulkanNativeBuffer
{
	VkBuffer buffer = nullptr;
	VkDeviceMemory memory = nullptr;
	VkMemoryRequirements memoryRequirements { };
};

namespace vulkan
{

VkCompareOp depthCompareOperation(CompareFunction);
VkFormat dataTypeValue(DataType);
VkPrimitiveTopology primitiveTopology(PrimitiveType);
VkCullModeFlags cullModeFlags(CullMode);
VkIndexType indexBufferFormat(IndexArrayFormat);
VkFormat textureFormatValue(TextureFormat);
VkImageType textureTargetToImageType(TextureTarget);
VkImageViewType textureTargetToImageViewType(TextureTarget);
uint32_t getMemoryTypeIndex(VulkanState& vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkSamplerAddressMode textureWrapToSamplerAddressMode(TextureWrap);
VkFilter textureFiltrationValueToFilter(TextureFiltration);
VkSamplerMipmapMode textureFiltrationValueToSamplerMipMapMode(TextureFiltration);
VkAttachmentLoadOp frameBufferOperationToLoadOperation(FramebufferOperation);
VkAttachmentStoreOp frameBufferOperationToStoreOperation(FramebufferOperation);

namespace gl
{
	DataType dataTypeFromOpenGLType(int glType);
	bool isSamplerType(int glType);
}

void imageBarrier(VulkanState&, VkCommandBuffer, VkImage, VkImageAspectFlags aspect,
	VkAccessFlags accessFrom, VkAccessFlags accessTo,
	VkImageLayout layoutFrom, VkImageLayout layoutTo,
	VkPipelineStageFlags stageFrom, VkPipelineStageFlags stageTo, 
	uint32_t startMipLevel = 0, uint32_t mipLevelsCount = 1);

}

const char* vulkanResultToString(VkResult result);

#define VULKAN_CALL(expr) do { \
	auto localVkResult = (expr); \
	if (localVkResult != VkResult::VK_SUCCESS) \
	{ \
		et::log::error("Vulkan call failed: %s\nat %s [%d]\nresult = %s", \
			(#expr), __FILE__, __LINE__, vulkanResultToString(localVkResult)); \
		et::debug::debugBreak(); \
	}} while (0)

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

}
