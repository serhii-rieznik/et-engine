/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/rendering.h>
#include <et/rendering/interface/program.h>
#include <mutex>

#if (ET_PLATFORM_WIN)
#	define VK_USE_PLATFORM_WIN32_KHR
#	include <et/rendering/vulkan/vulkan/vulkan.h>
#else
#	error Not implemented for this platform
#endif

#if (ET_DEBUG)
#	define ET_VULKAN_ENABLE_VALIDATION		1
#	define ET_VULKAN_ENABLE_DEBUG_MARKERS	1
#else
#	define ET_VULKAN_ENABLE_VALIDATION		0
#	define ET_VULKAN_ENABLE_DEBUG_MARKERS	1
#endif

namespace et {

struct VulkanState;
struct RenderContextParameters;

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
	VkQueueFamilyProperties properties = { };
};

class VulkanMemoryAllocatorPrivate;
class VulkanMemoryAllocator
{
public:
	struct Allocation
	{
		uint64_t id = 0;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceSize offset = 0;
		uint32_t mapCounter = 0;
		uint8_t* mappedData = nullptr;

		bool operator < (const Allocation& r) const { return id < r.id; }
	};
public:
	VulkanMemoryAllocator() = default;
	~VulkanMemoryAllocator();

	void init(VulkanState&);

	bool allocateSharedMemory(VkDeviceSize size, uint32_t type, Allocation&);
	bool allocateExclusiveMemory(VkDeviceSize size, uint32_t type, Allocation&);
	bool release(const Allocation&);

	uint8_t* map(const Allocation&);
	void unmap(const Allocation&);

private:
	ET_DECLARE_PIMPL(VulkanMemoryAllocator, 128);
};


class VulkanSwapchain
{
public:
	struct SwapchainFrame
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
		VkQueryPool timestampsQueryPool = nullptr;
		uint32_t timestampIndex = 0;
		uint32_t swapchainImageIndex = static_cast<uint32_t>(-1);
	};

public:
	void init(VulkanState& vulkan, const RenderContextParameters& params, HWND window);
	void createSizeDependentResources(VulkanState& vulkan, const vec2i&);

	void acquireFrameImage(SwapchainFrame&, VulkanState& vulkan);
	void present(SwapchainFrame&, VulkanState& vulkan);

	bool createDepthImage(VulkanState& vulkan, VkImage& image, VulkanMemoryAllocator::Allocation& allocation, VkCommandBuffer cmdBuffer);
	VkImageView createImageView(VulkanState&, VkImage, VkImageAspectFlags, VkFormat);

	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapchain = nullptr;

	VkExtent2D extent{};
	VkSurfaceFormatKHR surfaceFormat{};
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	struct DepthBuffer
	{
		VkImage image = nullptr;
		VkImageView imageView = nullptr;
		VulkanMemoryAllocator::Allocation memory;
	} depthBuffer;

	Vector<SwapchainFrame> frames;

	const SwapchainFrame& frame(uint64_t index) const {
		ET_ASSERT(index < RendererFrameCount);
		return frames[index];
	}

	SwapchainFrame& mutableFrame(uint64_t index) {
		ET_ASSERT(index < RendererFrameCount);
		return frames[index];
	}
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
	VulkanMemoryAllocator allocator;
	VulkanQueue queues[VulkanQueueClass_Count];

	VkCommandPool graphicsCommandPool = nullptr;
	VkCommandPool computeCommandPool = nullptr;

	using ServiceCommands = std::function<void(VkCommandBuffer)>;
	void executeServiceCommands(VulkanQueueClass, ServiceCommands);

	uint32_t writeTimestamp(VulkanSwapchain::SwapchainFrame& frame, VkCommandBuffer cmd, VkPipelineStageFlagBits stage);
};

struct VulkanShaderModules
{
	Map<ProgramStage, VkPipelineShaderStageCreateInfo> stageCreateInfos;
};

struct VulkanNativeRenderPass
{
	struct Content
	{
		VkSemaphore semaphore = nullptr;
		VkCommandBuffer commandBuffer = nullptr;
	};

	VkRenderPass renderPass = nullptr;
	VkDescriptorSetLayout dynamicDescriptorSetLayout = nullptr;
	VkDescriptorSet dynamicDescriptorSet = nullptr;
};

enum DescriptorSetClass : uint32_t
{
	Buffers,
	Textures,
	Samplers,
	Images,

	DescriptorSetClass_Count,
	DynamicDescriptorsCount = 2
};

struct VulkanNativePipeline
{
	VkPipeline pipeline = nullptr;
	VkPipelineLayout layout = nullptr;
	VkDescriptorSetLayout texturesSetLayout = nullptr;
	VkDescriptorSetLayout samplersSetLayout = nullptr;
	VkDescriptorSetLayout imagesSetLayout = nullptr;

	void buildLayout(VulkanState& vulkan, const Program::Reflection&, VkDescriptorSetLayout buffersSet);
	void cleanup(VulkanState& vulkan);
};

struct VulkanNativeTexture
{
	VulkanState& vulkan;
	VulkanNativeTexture(VulkanState& v) :
		vulkan(v) {
	}

	VkImage image = nullptr;
	VkMemoryRequirements memoryRequirements{};
	VulkanMemoryAllocator::Allocation allocation;

	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags aspect = VkImageAspectFlagBits::VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;

	Map<uint64_t, VkImageView> allImageViews;
	VkImageViewType imageViewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	uint32_t layerCount = 0;
	uint32_t levelCount = 0;

	VkImageView imageView(const ResourceRange&);
};

struct VulkanNativeSampler
{
	VkSampler sampler = nullptr;
};

struct VulkanNativeTextureSet
{
	VkDescriptorSetLayout texturesSetLayout = nullptr;
	VkDescriptorSet texturesSet = nullptr;
	VkDescriptorSetLayout samplersSetLayout = nullptr;
	VkDescriptorSet samplersSet = nullptr;
	VkDescriptorSetLayout imagesSetLayout = nullptr;
	VkDescriptorSet imagesSet = nullptr;
};

struct VulkanNativeBuffer
{
	VkBuffer buffer = nullptr;
	VulkanMemoryAllocator::Allocation allocation;
	VkMemoryRequirements memoryRequirements{};
};

namespace vulkan {

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
const char* programStageHLSLProfile(ProgramStage);
const char* resultToString(VkResult result);

VkPipelineStageFlags accessMaskToPipelineStage(VkAccessFlags flags);
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
	Vector<EnumeratedClass> operator ()(const Holder& holder, Function callable) {
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
	Vector<EnumeratedClass> operator ()(const Holder& holder, Function callable) {
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
Vector<EnumeratedClass> enumerateVulkanObjects(const Holder& holder, Function callable) {
	using ReturnClass = decltype(callable(holder, nullptr, nullptr));
	VulkanObjectsEnumerator<EnumeratedClass, Holder, Function, ReturnClass> enumerator;
	return enumerator(holder, callable);
}

}
