/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/rendering.h>
#include <et/rendering/interface/program.h>

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

	VkExtent2D extent{ };
	VkSurfaceFormatKHR surfaceFormat{ };
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	struct Frame
	{
		VkImage color = nullptr;
		VkImageView colorView = nullptr;

		VkCommandBuffer barrierFromPresent = nullptr;
		VkCommandBuffer barrierToPresent = nullptr;

		VkFence imageFence = nullptr;

		VkSemaphore imageAcquired = nullptr;
		VkSemaphore semaphoreFromPresent = nullptr;
		VkSemaphore semaphoreToPresent = nullptr;
		VkSemaphore submitCompleted = nullptr;
	};

	struct DepthBuffer
	{
		VkImage depth = nullptr;
		VkImageView depthView = nullptr;
		VkDeviceMemory depthMemory = nullptr;
	} depthBuffer;

	Vector<Frame> frames;
	uint32_t frameNumber = 0;

	uint32_t swapchainImageIndex = static_cast<uint32_t>(-1);

	const Frame& currentFrame() const
	{
		return frames.at(frameNumber % RendererFrameCount);
	}
};

enum VulkanQueueClass : uint32_t
{
	Graphics,
	Compute,

	VulkanQueueClass_Count
};

struct VulkanQueue
{
	uint32_t index = static_cast<uint32_t>(-1);

	VkQueue queue = nullptr;
	VkCommandPool commandPool = nullptr;
	VkCommandBuffer serviceCommandBuffer = nullptr;
};

struct VulkanState
{
	VkInstance instance = nullptr;
	VkDebugReportCallbackEXT debugCallback = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkDevice device = nullptr;
	VkDescriptorPool descriptorPool = nullptr;
	VkPipelineCache pipelineCache = nullptr;

	VkPhysicalDeviceProperties physicalDeviceProperties{ };
	VkPhysicalDeviceFeatures physicalDeviceFeatures{ };

	VulkanSwapchain swapchain;
	VulkanQueue queues[VulkanQueueClass_Count];

	VkCommandPool graphicsCommandPool = nullptr;
	VkCommandPool computeCommandPool = nullptr;

	using ServiceCommands = std::function<void(VkCommandBuffer)>;
	void executeServiceCommands(VulkanQueueClass, ServiceCommands);
};

struct VulkanShaderModules
{
	Map<ProgramStage, VkPipelineShaderStageCreateInfo> stageCreateInfos;
};

struct VulkanNativeRenderPass
{
	VkRenderPass renderPass = nullptr;
	VkDescriptorSetLayout dynamicDescriptorSetLayout = nullptr;
	VkDescriptorSet dynamicDescriptorSet = nullptr;

	struct RenderPassContent
	{
		VkSemaphore semaphore = nullptr;
		VkCommandBuffer commandBuffer = nullptr;
	};

	std::array<RenderPassContent, RendererFrameCount> content;
};

enum DescriptorSetClass : uint32_t
{
	Buffers,
	Textures,
	Images,

	DescriptorSetClass_Count,
	DynamicDescriptorsCount = 2
};

struct VulkanNativePipeline
{
	VkPipeline pipeline = nullptr;
	VkPipelineLayout layout = nullptr;
	VkDescriptorSetLayout textureSetLayout = nullptr;
	VkDescriptorSetLayout imageSetLayout = nullptr;

	void buildLayout(VulkanState& vulkan, const Program::Reflection&, VkDescriptorSetLayout buffersSet);
	void cleanup(VulkanState& vulkan);
};

struct VulkanNativeTexture
{
	VulkanState& vulkan;
	VulkanNativeTexture(VulkanState& v) :
		vulkan(v) { }

	VkImage image = nullptr;
	VkDeviceMemory memory = nullptr;
	VkMemoryRequirements memoryRequirements{ };
	// VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;

	Map<uint32_t, VkImageView> allImageViews;
	VkImageView completeImageView = nullptr;
	VkImageViewType imageViewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	uint32_t layerCount = 0;
	uint32_t levelCount = 0;

	uint32_t imageViewIndex(uint32_t layer, uint32_t level) const
	{
		return (layer & 0xffff) | ((level & 0xffff) << 16);
	}

	VkImageView imageView(uint32_t layer, uint32_t level);
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
	VkMemoryRequirements memoryRequirements{ };
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
VkBlendOp blendOperationValue(BlendOperation);
VkBlendFactor blendFactorValue(BlendFunction);
VkAccessFlags texureStateToAccessFlags(TextureState);
VkImageLayout texureStateToImageLayout(TextureState);
VkShaderStageFlagBits programStageValue(ProgramStage);
const char* programStageEntryName(ProgramStage);

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

const char* resultToString(VkResult result);
}


#define VULKAN_CALL(expr) do { \
	VkResult localVkResult = (expr); \
	if (localVkResult != VkResult::VK_SUCCESS) \
	{ \
		et::log::error("Vulkan call failed: %s\nat %s [%d]\nresult = %s", \
			(#expr), __FILE__, __LINE__, vulkan::resultToString(localVkResult)); \
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
