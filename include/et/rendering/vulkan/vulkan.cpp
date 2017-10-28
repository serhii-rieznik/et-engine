#include <Windows.h>
#include <gl/gl.h>
#include <et/rendering/opengl/gl/glext.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

#define CASE_TO_STRING(C) case C: return #C;

const char* vulkan::resultToString(VkResult result)
{
	switch (result)
	{
		CASE_TO_STRING(VK_SUCCESS)
			CASE_TO_STRING(VK_NOT_READY)
			CASE_TO_STRING(VK_TIMEOUT)
			CASE_TO_STRING(VK_EVENT_SET)
			CASE_TO_STRING(VK_EVENT_RESET)
			CASE_TO_STRING(VK_INCOMPLETE)
			CASE_TO_STRING(VK_ERROR_OUT_OF_HOST_MEMORY)
			CASE_TO_STRING(VK_ERROR_OUT_OF_DEVICE_MEMORY)
			CASE_TO_STRING(VK_ERROR_INITIALIZATION_FAILED)
			CASE_TO_STRING(VK_ERROR_DEVICE_LOST)
			CASE_TO_STRING(VK_ERROR_MEMORY_MAP_FAILED)
			CASE_TO_STRING(VK_ERROR_LAYER_NOT_PRESENT)
			CASE_TO_STRING(VK_ERROR_EXTENSION_NOT_PRESENT)
			CASE_TO_STRING(VK_ERROR_FEATURE_NOT_PRESENT)
			CASE_TO_STRING(VK_ERROR_INCOMPATIBLE_DRIVER)
			CASE_TO_STRING(VK_ERROR_TOO_MANY_OBJECTS)
			CASE_TO_STRING(VK_ERROR_FORMAT_NOT_SUPPORTED)
			CASE_TO_STRING(VK_ERROR_SURFACE_LOST_KHR)
			CASE_TO_STRING(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
			CASE_TO_STRING(VK_SUBOPTIMAL_KHR)
			CASE_TO_STRING(VK_ERROR_OUT_OF_DATE_KHR)
			CASE_TO_STRING(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
			CASE_TO_STRING(VK_ERROR_VALIDATION_FAILED_EXT)
			CASE_TO_STRING(VK_ERROR_INVALID_SHADER_NV)
	default:
		ET_FAIL_FMT("Unknown Vulkan error: %d", static_cast<int>(result));
	}
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHRWrapper(const VulkanState& state, uint32_t* count, VkSurfaceFormatKHR* formats)
{
	return vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, state.swapchain.surface, count, formats);
}

void VulkanState::executeServiceCommands(VulkanQueueClass cls, ServiceCommands commands)
{
	VulkanQueue& queue = queues[cls];

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VULKAN_CALL(vkBeginCommandBuffer(queue.serviceCommandBuffer, &beginInfo));
	{
		commands(queue.serviceCommandBuffer);
	}
	VULKAN_CALL(vkEndCommandBuffer(queue.serviceCommandBuffer));

	VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &queue.serviceCommandBuffer;
	VULKAN_CALL(vkQueueSubmit(queue.queue, 1, &submit, nullptr));

	VULKAN_CALL(vkQueueWaitIdle(queue.queue));
}

uint32_t VulkanState::writeTimestamp(VkCommandBuffer cmd, VkPipelineStageFlagBits stage)
{
	VulkanSwapchain::Frame& frame = swapchain.mutableCurrentFrame();

	uint32_t result = frame.timestampIndex++;
	vkCmdWriteTimestamp(cmd, stage, frame.timestampsQueryPool, result);
	return result;
}

/*
 *
 * Swapchain
 *
 */
void VulkanSwapchain::init(VulkanState& vulkan, const RenderContextParameters& params, HWND window)
{
	VkWin32SurfaceCreateInfoKHR surfaceInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfaceInfo.hinstance = GetModuleHandle(nullptr);
	surfaceInfo.hwnd = window;
	vkCreateWin32SurfaceKHR(vulkan.instance, &surfaceInfo, nullptr, &surface);

	VkBool32 graphicsQueueSupportsPresent = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(vulkan.physicalDevice, vulkan.queues[VulkanQueueClass::Graphics].index, surface, &graphicsQueueSupportsPresent);
	ET_ASSERT(graphicsQueueSupportsPresent);

	Vector<VkSurfaceFormatKHR> formats = enumerateVulkanObjects<VkSurfaceFormatKHR>(vulkan, vkGetPhysicalDeviceSurfaceFormatsKHRWrapper);
	ET_ASSERT(formats.size() > 0);
	surfaceFormat = formats.front();
}

VkResult vkGetPhysicalDeviceSurfacePresentModesKHRWrapper(const VulkanState& state, uint32_t* count, VkPresentModeKHR* modes)
{
	return vkGetPhysicalDeviceSurfacePresentModesKHR(state.physicalDevice, state.swapchain.surface, count, modes);
}

VkResult vkGetSwapchainImagesKHRWrapper(const VulkanState& state, uint32_t* count, VkImage* images)
{
	return vkGetSwapchainImagesKHR(state.device, state.swapchain.swapchain, count, images);
}

bool VulkanSwapchain::createDepthImage(VulkanState& vulkan, VkImage& image, VulkanMemoryAllocator::Allocation& allocation, VkCommandBuffer cmdBuffer)
{
	VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.format = depthFormat;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.extent.width = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkFormatProperties formatProperties = {};
	vkGetPhysicalDeviceFormatProperties(vulkan.physicalDevice, imageInfo.format, &formatProperties);
	if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	else if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	else
		return false;

	VULKAN_CALL(vkCreateImage(vulkan.device, &imageInfo, nullptr, &image));

	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(vulkan.device, image, &memoryRequirements);

	uint32_t typeIndex = vulkan::getMemoryTypeIndex(vulkan, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vulkan.allocator.allocateExclusiveMemory(memoryRequirements.size, typeIndex, allocation))
	{
		VULKAN_CALL(vkBindImageMemory(vulkan.device, image, allocation.memory, allocation.offset));
	}

	VkImageMemoryBarrier barrierInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrierInfo.srcAccessMask = 0;
	barrierInfo.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barrierInfo.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrierInfo.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrierInfo.srcQueueFamilyIndex = vulkan.queues[VulkanQueueClass::Graphics].index;
	barrierInfo.dstQueueFamilyIndex = vulkan.queues[VulkanQueueClass::Graphics].index;
	barrierInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
	barrierInfo.image = image;
	vkCmdPipelineBarrier(cmdBuffer, vulkan::accessMaskToPipelineStage(barrierInfo.srcAccessMask),
		vulkan::accessMaskToPipelineStage(barrierInfo.dstAccessMask), 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);

	return true;
}

VkImageView VulkanSwapchain::createImageView(VulkanState& vulkan, VkImage image, VkImageAspectFlags aspect, VkFormat format)
{
	VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.image = image;
	viewInfo.format = format;
	viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.subresourceRange = { aspect, 0, 1, 0, 1 };

	VkImageView result = nullptr;
	VULKAN_CALL(vkCreateImageView(vulkan.device, &viewInfo, nullptr, &result));
	return result;
}

void VulkanSwapchain::createSizeDependentResources(VulkanState& vulkan, const vec2i& sz)
{
	VULKAN_CALL(vkDeviceWaitIdle(vulkan.device));

	extent.width = static_cast<uint32_t>(sz.x);
	extent.height = static_cast<uint32_t>(sz.y);

	VkSurfaceCapabilitiesKHR surfaceCaps = { };
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan.physicalDevice, surface, &surfaceCaps);

	uint32_t numImages = surfaceCaps.minImageCount + 1;
	if ((surfaceCaps.maxImageCount > 0) && (numImages > surfaceCaps.maxImageCount))
		numImages = surfaceCaps.maxImageCount;

	VkSurfaceTransformFlagsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	ET_ASSERT(surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR);

	Vector<VkPresentModeKHR> presentModes = enumerateVulkanObjects<VkPresentModeKHR>(vulkan, vkGetPhysicalDeviceSurfacePresentModesKHRWrapper);
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

	VkSwapchainKHR currentSwapchain = swapchain;

	VkSwapchainCreateInfoKHR swapchainInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainInfo.surface = surface;
	swapchainInfo.minImageCount = numImages;
	swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainInfo.imageFormat = surfaceFormat.format;
	swapchainInfo.imageExtent = extent;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(preTransform);
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.oldSwapchain = currentSwapchain;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VULKAN_CALL(vkCreateSwapchainKHR(vulkan.device, &swapchainInfo, nullptr, &swapchain));

	VkCommandPool graphicsCommandPool = vulkan.queues[VulkanQueueClass::Graphics].commandPool;

	if (currentSwapchain != nullptr)
	{
		for (Frame& frame : frames)
		{
			vkDestroyImageView(vulkan.device, frame.colorView, nullptr);
			frame.colorView = nullptr;

			vkFreeCommandBuffers(vulkan.device, graphicsCommandPool, 1, &frame.barrierFromPresent);
			frame.barrierFromPresent = nullptr;
			vkFreeCommandBuffers(vulkan.device, graphicsCommandPool, 1, &frame.barrierToPresent);
			frame.barrierToPresent = nullptr;

			vkDestroySemaphore(vulkan.device, frame.semaphoreFromPresent, nullptr);
			frame.semaphoreFromPresent = nullptr;
			vkDestroySemaphore(vulkan.device, frame.semaphoreToPresent, nullptr);
			frame.semaphoreToPresent = nullptr;
			vkDestroySemaphore(vulkan.device, frame.imageAcquired, nullptr);
			frame.imageAcquired = nullptr;
			vkDestroySemaphore(vulkan.device, frame.submitCompleted, nullptr);
			frame.submitCompleted = nullptr;

			vkDestroyQueryPool(vulkan.device, frame.timestampsQueryPool, nullptr);
			frame.timestampsQueryPool = nullptr;

			vkDestroyFence(vulkan.device, frame.imageFence, nullptr);
			frame.imageFence = nullptr;
		}

		vkDestroyImageView(vulkan.device, depthBuffer.imageView, nullptr);
		depthBuffer.imageView = nullptr;

		vkDestroyImage(vulkan.device, depthBuffer.image, nullptr);
		depthBuffer.image = nullptr;

		vulkan.allocator.release(depthBuffer.memory);
		depthBuffer.memory = { };

		vkDestroySwapchainKHR(vulkan.device, currentSwapchain, nullptr);
	}

	Vector<VkImage> swapchainImages = enumerateVulkanObjects<VkImage>(vulkan, vkGetSwapchainImagesKHRWrapper);
	frames.resize(swapchainImages.size());

	VkImage* swapchainImagesPtr = swapchainImages.data();
	for (Frame& frame : frames)
	{
		frame.color = *swapchainImagesPtr++;
		frame.colorView = createImageView(vulkan, frame.color, VK_IMAGE_ASPECT_COLOR_BIT, surfaceFormat.format);

		VkQueryPoolCreateInfo queryPoolInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolInfo.queryCount = 1024;
		VULKAN_CALL(vkCreateQueryPool(vulkan.device, &queryPoolInfo, nullptr, &frame.timestampsQueryPool));

		VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandBufferCount = 1;
		allocInfo.commandPool = graphicsCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VULKAN_CALL(vkAllocateCommandBuffers(vulkan.device, &allocInfo, &frame.barrierFromPresent));
		VULKAN_CALL(vkAllocateCommandBuffers(vulkan.device, &allocInfo, &frame.barrierToPresent));

		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VULKAN_CALL(vkBeginCommandBuffer(frame.barrierFromPresent, &beginInfo));
		{
			VkImageMemoryBarrier barrierInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barrierInfo.srcAccessMask = 0;
			barrierInfo.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrierInfo.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrierInfo.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrierInfo.srcQueueFamilyIndex = vulkan.queues[VulkanQueueClass::Graphics].index;
			barrierInfo.dstQueueFamilyIndex = vulkan.queues[VulkanQueueClass::Graphics].index;
			barrierInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			barrierInfo.image = frame.color;
			vkCmdPipelineBarrier(frame.barrierFromPresent, vulkan::accessMaskToPipelineStage(barrierInfo.srcAccessMask),
				vulkan::accessMaskToPipelineStage(barrierInfo.dstAccessMask), 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
			vkCmdResetQueryPool(frame.barrierFromPresent, frame.timestampsQueryPool, 0, 1024);
		}
		VULKAN_CALL(vkEndCommandBuffer(frame.barrierFromPresent));

		VULKAN_CALL(vkBeginCommandBuffer(frame.barrierToPresent, &beginInfo));
		{
			VkImageMemoryBarrier barrierInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barrierInfo.srcAccessMask = 0;
			barrierInfo.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrierInfo.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrierInfo.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrierInfo.srcQueueFamilyIndex = vulkan.queues[VulkanQueueClass::Graphics].index;
			barrierInfo.dstQueueFamilyIndex = vulkan.queues[VulkanQueueClass::Graphics].index;
			barrierInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			barrierInfo.image = frame.color;
			vkCmdPipelineBarrier(frame.barrierToPresent, vulkan::accessMaskToPipelineStage(barrierInfo.srcAccessMask),
				vulkan::accessMaskToPipelineStage(barrierInfo.dstAccessMask), 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
		}
		VULKAN_CALL(vkEndCommandBuffer(frame.barrierToPresent));

		VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VULKAN_CALL(vkCreateSemaphore(vulkan.device, &semaphoreInfo, nullptr, &frame.semaphoreFromPresent));
		VULKAN_CALL(vkCreateSemaphore(vulkan.device, &semaphoreInfo, nullptr, &frame.semaphoreToPresent));
		VULKAN_CALL(vkCreateSemaphore(vulkan.device, &semaphoreInfo, nullptr, &frame.imageAcquired));
		VULKAN_CALL(vkCreateSemaphore(vulkan.device, &semaphoreInfo, nullptr, &frame.submitCompleted));

		VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VULKAN_CALL(vkCreateFence(vulkan.device, &fenceInfo, nullptr, &frame.imageFence));
	}

	vulkan.executeServiceCommands(VulkanQueueClass::Graphics, [&](VkCommandBuffer cmdBuffer) {
		if (createDepthImage(vulkan, depthBuffer.image, depthBuffer.memory, cmdBuffer)) {
			depthBuffer.imageView = createImageView(vulkan, depthBuffer.image, VK_IMAGE_ASPECT_DEPTH_BIT, depthFormat);
		}
	});

	VULKAN_CALL(vkDeviceWaitIdle(vulkan.device));
}

void VulkanSwapchain::acquireNextImage(VulkanState& vulkan)
{
	VkFence currentFence = frames[frameNumber % RendererFrameCount].imageFence;
	VULKAN_CALL(vkWaitForFences(vulkan.device, 1, &currentFence, VK_TRUE, UINT64_MAX));
	VULKAN_CALL(vkResetFences(vulkan.device, 1, &currentFence));

	VULKAN_CALL(vkAcquireNextImageKHR(vulkan.device, swapchain, UINT64_MAX,
		frames[frameNumber % RendererFrameCount].imageAcquired, nullptr, &swapchainImageIndex));
}

void VulkanSwapchain::present(VulkanState& vulkan)
{
	VkPresentInfoKHR info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	info.swapchainCount = 1;
	info.pSwapchains = &swapchain;
	info.pImageIndices = &swapchainImageIndex;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &frames[frameNumber % RendererFrameCount].submitCompleted;
	VULKAN_CALL(vkQueuePresentKHR(vulkan.queues[VulkanQueueClass::Graphics].queue, &info));

	++frameNumber;
}

void VulkanNativePipeline::buildLayout(VulkanState& vulkan, const Program::Reflection& reflection, VkDescriptorSetLayout buffersSet)
{
	Vector<VkDescriptorSetLayoutBinding> textureBindings;
	textureBindings.reserve(2 * MaterialTexture_max);

	Vector<VkDescriptorSetLayoutBinding> imageBindings;
	imageBindings.reserve(StorageBuffer_max);

	for (const auto& tex : reflection.textures)
	{
		VkShaderStageFlagBits stageFlags = vulkan::programStageValue(tex.stage);

		for (const auto& t : tex.textures)
		{
			textureBindings.emplace_back();
			textureBindings.back().binding = t.second;
			textureBindings.back().stageFlags = stageFlags;
			textureBindings.back().descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			textureBindings.back().descriptorCount = 1;
		}
		for (const auto& t : tex.samplers)
		{
			textureBindings.emplace_back();
			textureBindings.back().binding = t.second;
			textureBindings.back().stageFlags = stageFlags;
			textureBindings.back().descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			textureBindings.back().descriptorCount = 1;
		}
		for (const auto& t : tex.images)
		{
			imageBindings.emplace_back();
			imageBindings.back().binding = t.second;
			imageBindings.back().stageFlags = stageFlags;
			imageBindings.back().descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			imageBindings.back().descriptorCount = 1;
		}
	}

	VkDescriptorSetLayoutCreateInfo layoutSetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutSetInfo.bindingCount = static_cast<uint32_t>(textureBindings.size());
	layoutSetInfo.pBindings = textureBindings.data();
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &layoutSetInfo, nullptr, &textureSetLayout));

	layoutSetInfo.bindingCount = static_cast<uint32_t>(imageBindings.size());
	layoutSetInfo.pBindings = imageBindings.data();
	VULKAN_CALL(vkCreateDescriptorSetLayout(vulkan.device, &layoutSetInfo, nullptr, &imageSetLayout));

	VkDescriptorSetLayout layouts[] =
	{
		buffersSet,
		textureSetLayout,
		imageSetLayout
	};

	VkPipelineLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutCreateInfo.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
	layoutCreateInfo.pSetLayouts = layouts;
	VULKAN_CALL(vkCreatePipelineLayout(vulkan.device, &layoutCreateInfo, nullptr, &layout));
}

void VulkanNativePipeline::cleanup(VulkanState& vulkan)
{
	if (pipeline)
		vkDestroyPipeline(vulkan.device, pipeline, nullptr);

	if (layout)
		vkDestroyPipelineLayout(vulkan.device, layout, nullptr);

	if (textureSetLayout)
		vkDestroyDescriptorSetLayout(vulkan.device, textureSetLayout, nullptr);

	if (imageSetLayout)
		vkDestroyDescriptorSetLayout(vulkan.device, imageSetLayout, nullptr);
}

namespace vulkan
{

uint32_t getMemoryTypeIndex(VulkanState& vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	static bool propertiesRetreived = false;
	static VkPhysicalDeviceMemoryProperties memProperties = {};

	if (propertiesRetreived == false)
	{
		vkGetPhysicalDeviceMemoryProperties(vulkan.physicalDevice, &memProperties);
		propertiesRetreived = true;
	}

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	ET_FAIL("Unable to get memory type");
}

VkCompareOp depthCompareOperation(CompareFunction func)
{
	switch (func)
	{
	case CompareFunction::Always:
		return VkCompareOp::VK_COMPARE_OP_ALWAYS;
	case CompareFunction::Equal:
		return VkCompareOp::VK_COMPARE_OP_EQUAL;
	case CompareFunction::Greater:
		return VkCompareOp::VK_COMPARE_OP_GREATER;
	case CompareFunction::GreaterOrEqual:
		return VkCompareOp::VK_COMPARE_OP_GREATER_OR_EQUAL;
	case CompareFunction::Less:
		return VkCompareOp::VK_COMPARE_OP_LESS;
	case CompareFunction::LessOrEqual:
		return VkCompareOp::VK_COMPARE_OP_LESS_OR_EQUAL;
	case CompareFunction::Never:
		return VkCompareOp::VK_COMPARE_OP_NEVER;
	default:
		ET_FAIL("Invalid CompareFunction")
	}
}

VkFormat dataTypeValue(DataType fmt)
{
	switch (fmt)
	{
	case DataType::Float: return VK_FORMAT_R32_SFLOAT;
	case DataType::Vec2: return VK_FORMAT_R32G32_SFLOAT;
	case DataType::Vec3: return VK_FORMAT_R32G32B32_SFLOAT;
	case DataType::Vec4: return VK_FORMAT_R32G32B32A32_SFLOAT;
	case DataType::Int: return VK_FORMAT_R32_SINT;
	case DataType::IntVec2: return VK_FORMAT_R32G32_SINT;
	case DataType::IntVec3: return VK_FORMAT_R32G32B32_SINT;
	case DataType::IntVec4: return VK_FORMAT_R32G32B32A32_SINT;
	default:
		ET_ASSERT(!"Invalid data type provided");
		return VK_FORMAT_UNDEFINED;
	};
}

VkFormat textureFormatValue(TextureFormat fmt)
{
	switch (fmt)
	{
	case TextureFormat::R8: return VK_FORMAT_R8_UNORM;
	case TextureFormat::R16: return VK_FORMAT_R16_UNORM;
	case TextureFormat::RGBA8: return  VK_FORMAT_R8G8B8A8_UNORM;
	case TextureFormat::BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
	case TextureFormat::RGBA16: return VK_FORMAT_R16G16B16A16_UNORM;
	case TextureFormat::R16F: return VK_FORMAT_R16_SFLOAT;
	case TextureFormat::RG16F: return VK_FORMAT_R16G16_SFLOAT;
	case TextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
	case TextureFormat::R32F: return VK_FORMAT_R32_SFLOAT;
	case TextureFormat::RG32F: return VK_FORMAT_R32G32_SFLOAT;
	case TextureFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
	case TextureFormat::Depth16: return VK_FORMAT_D16_UNORM;
	case TextureFormat::Depth24: return VK_FORMAT_D24_UNORM_S8_UINT;
	case TextureFormat::Depth32F: return VK_FORMAT_D32_SFLOAT;
	case TextureFormat::DXT5: return VK_FORMAT_BC3_UNORM_BLOCK;
	default:
		ET_ASSERT(!"Invalid TextureFormat provided");
		return VK_FORMAT_UNDEFINED;
	};
}

VkImageType textureTargetToImageType(TextureTarget target)
{
	switch (target)
	{
	case TextureTarget::Texture_2D:
	case TextureTarget::Texture_Cube:
	case TextureTarget::Texture_2D_Array:
		return VkImageType::VK_IMAGE_TYPE_2D;

	default:
		ET_ASSERT(!"Invalid TextureTarget specified");
		return VK_IMAGE_TYPE_MAX_ENUM;
	}
}

VkImageViewType textureTargetToImageViewType(TextureTarget target)
{
	switch (target)
	{
	case TextureTarget::Texture_2D:
		return VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
	case TextureTarget::Texture_2D_Array:
		return VkImageViewType::VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	case TextureTarget::Texture_Cube:
		return VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;
	default:
		ET_FAIL("Invalid TextureTarget specified");
	}
}

VkPrimitiveTopology primitiveTopology(PrimitiveType type)
{
	switch (type)
	{
	case PrimitiveType::Points: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case PrimitiveType::Lines: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case PrimitiveType::LineStrips: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case PrimitiveType::LinesAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
	case PrimitiveType::LineStripAdjacency: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
	case PrimitiveType::Triangles: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case PrimitiveType::TriangleStrips: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	default:
		ET_ASSERT(!"Invalid PrimitiveType provided");
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	};
}

VkCullModeFlags cullModeFlags(CullMode mode)
{
	switch (mode)
	{
	case CullMode::Disabled:
		return VkCullModeFlagBits::VK_CULL_MODE_NONE;
	case CullMode::Front:
		return VkCullModeFlagBits::VK_CULL_MODE_FRONT_BIT;
	case CullMode::Back:
		return VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
	default:
		ET_ASSERT(!"Invalid CullMode");
		return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
	}
}

VkIndexType indexBufferFormat(IndexArrayFormat fmt)
{
	switch (fmt)
	{
	case IndexArrayFormat::Format_16bit:
		return VkIndexType::VK_INDEX_TYPE_UINT16;
	case IndexArrayFormat::Format_32bit:
		return VkIndexType::VK_INDEX_TYPE_UINT32;
	default:
		ET_ASSERT(!"Invalid IndexArrayFormat");
		return VK_INDEX_TYPE_MAX_ENUM;
	}
}

VkSamplerAddressMode textureWrapToSamplerAddressMode(TextureWrap wrap)
{
	switch (wrap)
	{
	case TextureWrap::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case TextureWrap::MirrorRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case TextureWrap::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	default:
		ET_ASSERT(!"Invalid TextureWrap provided");
		return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	};
}

VkFilter textureFiltrationValue(TextureFiltration flt)
{
	switch (flt)
	{
	case TextureFiltration::Nearest: return VK_FILTER_NEAREST;
	case TextureFiltration::Linear: return VK_FILTER_LINEAR;
	default:
		ET_ASSERT(!"Invalid TextureFiltration provided");
		return VK_FILTER_MAX_ENUM;
	};
}

VkSamplerMipmapMode textureFiltrationValueToSamplerMipMapMode(TextureFiltration flt)
{
	switch (flt)
	{
	case TextureFiltration::Nearest: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case TextureFiltration::Linear: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	default:
		ET_ASSERT(!"Invalid TextureFiltration provided");
		return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
	};
}

VkAttachmentLoadOp frameBufferOperationToLoadOperation(FramebufferOperation val)
{
	switch (val)
	{
	case FramebufferOperation::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case FramebufferOperation::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
	case FramebufferOperation::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	default:
		ET_ASSERT(!"Invalid FramebufferOperation provided");
		return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
	};
}

VkAttachmentStoreOp frameBufferOperationToStoreOperation(FramebufferOperation val)
{
	switch (val)
	{
	case FramebufferOperation::Store: return VK_ATTACHMENT_STORE_OP_STORE;
	case FramebufferOperation::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	default:
		ET_ASSERT(!"Invalid FramebufferOperation provided");
		return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
	};
}

VkBlendOp blendOperationValue(BlendOperation val)
{
	switch (val)
	{
	case BlendOperation::Add: return VK_BLEND_OP_ADD;
	case BlendOperation::Subtract: return VK_BLEND_OP_SUBTRACT;
	case BlendOperation::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
	default:
		ET_ASSERT(!"Invalid BlendOperation provided");
		return VK_BLEND_OP_MAX_ENUM;
	};
}

VkBlendFactor blendFactorValue(BlendFunction val)
{
	switch (val)
	{
	case BlendFunction::Zero: return VK_BLEND_FACTOR_ZERO;
	case BlendFunction::One: return VK_BLEND_FACTOR_ONE;
	case BlendFunction::SourceColor: return VK_BLEND_FACTOR_SRC_COLOR;
	case BlendFunction::InvSourceColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case BlendFunction::SourceAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
	case BlendFunction::InvSourceAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case BlendFunction::DestColor: return VK_BLEND_FACTOR_DST_COLOR;
	case BlendFunction::InvDestColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case BlendFunction::DestAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
	case BlendFunction::InvDestAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	default:
		ET_ASSERT(!"Invalid BlendFunction provided");
		return VK_BLEND_FACTOR_MAX_ENUM;
	};
}

VkAccessFlags texureStateToAccessFlags(TextureState val)
{
	switch (val)
	{
	case TextureState::Undefined: return 0;
	case TextureState::CopySource: return VK_ACCESS_TRANSFER_READ_BIT;
	case TextureState::CopyDestination: return VK_ACCESS_TRANSFER_WRITE_BIT;
	case TextureState::ColorRenderTarget: return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	case TextureState::DepthRenderTarget: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	case TextureState::ShaderResource: return VK_ACCESS_SHADER_READ_BIT;
	case TextureState::PresentImage: return VK_ACCESS_MEMORY_READ_BIT;
	case TextureState::Storage: return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	default:
		ET_ASSERT(!"Invalid TextureState provided");
		return 0;
	};
}

VkPipelineStageFlags accessMaskToPipelineStage(VkAccessFlags flags)
{
	// shamelessly stolen from validation layers, 
	// supposed to be correct
	static const std::vector<std::pair<VkAccessFlags, VkPipelineStageFlags>> values =
	{
		{ VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
			VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT },

		{ VK_ACCESS_INDEX_READ_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT },

		{ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT },

		{ VK_ACCESS_UNIFORM_READ_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT /* | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT */ | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT },

		{ VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT },

		{ VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | /* | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | */
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT },

		{ VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | /* | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
			VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | */
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT },

		{ VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },

		{ VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },

		{ VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },

		{ VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT },

		{ VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT },

		{ VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT },

		{ VK_ACCESS_HOST_READ_BIT,
			VK_PIPELINE_STAGE_HOST_BIT },

		{ VK_ACCESS_HOST_WRITE_BIT,
			VK_PIPELINE_STAGE_HOST_BIT },

		{ VK_ACCESS_MEMORY_READ_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },

		{ VK_ACCESS_MEMORY_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT },

		{ VK_ACCESS_COMMAND_PROCESS_READ_BIT_NVX,
			VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX },

		{ VK_ACCESS_COMMAND_PROCESS_WRITE_BIT_NVX,
			VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX },
	};

	VkPipelineStageFlags result = 0;
	for (const auto& p : values)
	{
		if ((p.first & flags) != 0)
			result |= p.second;
	}

	return (result == 0) ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : result;
}

VkImageLayout texureStateToImageLayout(TextureState val)
{
	switch (val)
	{
	case TextureState::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
	case TextureState::CopySource: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	case TextureState::CopyDestination: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case TextureState::ColorRenderTarget: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case TextureState::DepthRenderTarget: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case TextureState::ShaderResource: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case TextureState::PresentImage: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	case TextureState::Storage: return VK_IMAGE_LAYOUT_GENERAL;
	default:
		ET_ASSERT(!"Invalid TextureState provided");
		return VK_IMAGE_LAYOUT_MAX_ENUM;
	};
}

VkShaderStageFlagBits programStageValue(ProgramStage val)
{
	switch (val)
	{
	case ProgramStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
	case ProgramStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
	case ProgramStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
	default:
		ET_ASSERT(!"Invalid ProgramStage provided");
		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	};
}

const char* programStageHLSLProfile(ProgramStage val) {
	switch (val)
	{
	case ProgramStage::Vertex: return "vs_5_1";
	case ProgramStage::Fragment: return "ps_5_1";
	case ProgramStage::Compute: return "cs_5_1";
	default:
		ET_ASSERT(!"Invalid ProgramStage provided");
		return "???";
	};
}
const char* programStageEntryName(ProgramStage val)
{
	switch (val)
	{
	case ProgramStage::Vertex: return "vertexMain";
	case ProgramStage::Fragment: return "fragmentMain";
	case ProgramStage::Compute: return "computeMain";
	default:
		ET_ASSERT(!"Invalid ProgramStage provided");
		return "???";
	};
}

}

VkImageView VulkanNativeTexture::imageView(const ResourceRange& range)
{
	uint64_t hsh = range.hash();
	VkImageView imageView = allImageViews[hsh];
	if (imageView == nullptr)
	{
		uint32_t levels = range.levelCount == std::numeric_limits<uint32_t>::max() ? VK_REMAINING_MIP_LEVELS : range.levelCount;
		uint32_t layers = range.layerCount == std::numeric_limits<uint32_t>::max() ? VK_REMAINING_ARRAY_LAYERS : range.layerCount;

		ET_ASSERT((levels == VK_REMAINING_MIP_LEVELS) || (levels <= levelCount));

		ET_ASSERT((layers == VK_REMAINING_ARRAY_LAYERS) || (layers <= layerCount));

		ET_ASSERT
		(
			(imageViewType != VK_IMAGE_VIEW_TYPE_CUBE) ||
			(imageViewType == VK_IMAGE_VIEW_TYPE_CUBE) && ((layers == 6) || (layers == VK_REMAINING_ARRAY_LAYERS))
		);

		VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		imageViewInfo.image = image;
		imageViewInfo.viewType = imageViewType;
		imageViewInfo.format = format;
		imageViewInfo.subresourceRange = { aspect, range.firstLevel, levels, range.firstLayer, layers };
		VULKAN_CALL(vkCreateImageView(vulkan.device, &imageViewInfo, nullptr, &imageView));
		allImageViews[hsh] = imageView;
	}
	return imageView;
}

}
