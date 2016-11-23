#include <Windows.h>
#include <gl/gl.h>
#include <et/rendering/opengl/gl/glext.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

#define CASE_TO_STRING(C) case C: return #C;

const char* vulkanResultToString(VkResult result)
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

	return nullptr;
}

VkResult vkGetPhysicalDeviceSurfaceFormatsKHRWrapper(const VulkanState& state, uint32_t* count, VkSurfaceFormatKHR* formats)
{
	return vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, state.swapchain.surface, count, formats);
}

void VulkanState::executeServiceCommands(ServiceCommands commands)
{
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &serviceCommandBuffer;

	VULKAN_CALL(vkBeginCommandBuffer(serviceCommandBuffer, &beginInfo));
	commands(serviceCommandBuffer);
	VULKAN_CALL(vkEndCommandBuffer(serviceCommandBuffer));
	VULKAN_CALL(vkQueueSubmit(queue, 1, &submit, nullptr));
	VULKAN_CALL(vkQueueWaitIdle(queue));
}

/*
 * Swapchain
 */

void VulkanSwapchain::init(VulkanState& vulkan, const RenderContextParameters& params, HWND window)
{
	VkWin32SurfaceCreateInfoKHR surfaceInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfaceInfo.hinstance = GetModuleHandle(nullptr);
	surfaceInfo.hwnd = window;
	vkCreateWin32SurfaceKHR(vulkan.instance, &surfaceInfo, nullptr, &surface);

	for (uint32_t i = 0, e = static_cast<uint32_t>(vulkan.queueProperties.size()); i < e; ++i)
	{
		VkBool32 supported = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(vulkan.physicalDevice, vulkan.graphicsQueueIndex, surface, &supported);
		if (supported)
		{
			vulkan.presentQueueIndex = i;
			break;
		}
	}
	ET_ASSERT(vulkan.presentQueueIndex == vulkan.graphicsQueueIndex);

	auto formats = enumerateVulkanObjects<VkSurfaceFormatKHR>(vulkan, vkGetPhysicalDeviceSurfaceFormatsKHRWrapper);
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

bool VulkanSwapchain::createDepthImage(VulkanState& vulkan, VkImage& image, VkDeviceMemory& memory, VkCommandBuffer cmdBuffer)
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

	VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = vulkan::getMemoryTypeIndex(vulkan, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VULKAN_CALL(vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &memory));
	VULKAN_CALL(vkBindImageMemory(vulkan.device, image, memory, 0));

	vulkan::imageBarrier(vulkan, cmdBuffer, image, VK_IMAGE_ASPECT_DEPTH_BIT,
		0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

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

	if (currentSwapchain != nullptr)
	{
		for (RenderTarget& rt : images)
		{
			vkDestroyImageView(vulkan.device, rt.colorView, nullptr);
			rt.colorView = nullptr;

			vkDestroyImageView(vulkan.device, rt.depthView, nullptr);
			rt.depthView = nullptr;
			
			vkDestroyImage(vulkan.device, rt.depth, nullptr);
			rt.depth = nullptr;
			
			vkFreeMemory(vulkan.device, rt.depthMemory, nullptr);
			rt.depthMemory = nullptr;
		}
		vkDestroySwapchainKHR(vulkan.device, currentSwapchain, nullptr);
	}

	Vector<VkImage> swapchainImages = enumerateVulkanObjects<VkImage>(vulkan, vkGetSwapchainImagesKHRWrapper);
	images.resize(swapchainImages.size());

	vulkan.executeServiceCommands([&](VkCommandBuffer cmdBuffer)
	{
		VkImage* swapchainImagesPtr = swapchainImages.data();
		for (RenderTarget& rt : images)
		{
			rt.color = *swapchainImagesPtr++;
			rt.colorView = createImageView(vulkan, rt.color, VK_IMAGE_ASPECT_COLOR_BIT, surfaceFormat.format);

			if (createDepthImage(vulkan, rt.depth, rt.depthMemory, cmdBuffer))
			{
				rt.depthView = createImageView(vulkan, rt.depth, VK_IMAGE_ASPECT_DEPTH_BIT, depthFormat);
			}
		}
	});
}

void VulkanSwapchain::acquireNextImage(VulkanState& vulkan)
{
	VULKAN_CALL(vkAcquireNextImageKHR(vulkan.device, swapchain, UINT64_MAX,
		vulkan.semaphores.imageAvailable, nullptr, &currentImageIndex));
}

void VulkanSwapchain::present(VulkanState& vulkan)
{
	VkPresentInfoKHR info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	info.swapchainCount = 1;
	info.pSwapchains = &swapchain;
	info.pImageIndices = &currentImageIndex;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &vulkan.semaphores.renderComplete;
	VULKAN_CALL(vkQueuePresentKHR(vulkan.queue, &info));
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

void imageBarrier(VulkanState& vulkan, VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect,
	VkAccessFlags accessFrom, VkAccessFlags accessTo, VkImageLayout layoutFrom, VkImageLayout layoutTo,
	VkPipelineStageFlags stageFrom, VkPipelineStageFlags stageTo, uint32_t startMipLevel, uint32_t mipLevelsCount)
{
	VkImageMemoryBarrier barrierInfo = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrierInfo.srcAccessMask = accessFrom;
	barrierInfo.dstAccessMask = accessTo;
	barrierInfo.oldLayout = layoutFrom;
	barrierInfo.newLayout = layoutTo;
	barrierInfo.srcQueueFamilyIndex = vulkan.presentQueueIndex;
	barrierInfo.dstQueueFamilyIndex = vulkan.graphicsQueueIndex;
	barrierInfo.subresourceRange = { aspect, startMipLevel, mipLevelsCount, 0, 1 };
	barrierInfo.image = image;
	vkCmdPipelineBarrier(cmd, stageFrom, stageTo, 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
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
	static const Map<DataType, VkFormat> lookup =
	{
		{ DataType::Float, VkFormat::VK_FORMAT_R32_SFLOAT },
		{ DataType::Vec2, VkFormat::VK_FORMAT_R32G32_SFLOAT },
		{ DataType::Vec3, VkFormat::VK_FORMAT_R32G32B32_SFLOAT },
		{ DataType::Vec4, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT },
		{ DataType::Int, VkFormat::VK_FORMAT_R32_SINT },
		{ DataType::IntVec2, VkFormat::VK_FORMAT_R32G32_SINT },
		{ DataType::IntVec3, VkFormat::VK_FORMAT_R32G32B32_SINT },
		{ DataType::IntVec4, VkFormat::VK_FORMAT_R32G32B32A32_SINT },
	};
	ET_ASSERT(lookup.count(fmt) > 0);
	return lookup.at(fmt);
}

VkFormat textureFormatValue(TextureFormat fmt)
{
	static const Map<TextureFormat, VkFormat> lookup =
	{
		{ TextureFormat::RGBA8, VkFormat::VK_FORMAT_R8G8B8A8_UNORM },
		{ TextureFormat::BGRA8, VkFormat::VK_FORMAT_B8G8R8A8_UNORM },
		{ TextureFormat::RGBA32F, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT },
	};
	ET_ASSERT(lookup.count(fmt) > 0);
	return lookup.at(fmt);
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
		ET_FAIL("Invalid TextureTarget specified");
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
	static const Map<PrimitiveType, VkPrimitiveTopology> lookup =
	{
		{ PrimitiveType::Points, VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST },
		{ PrimitiveType::Lines, VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST },
		{ PrimitiveType::LineStrips, VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP },
		{ PrimitiveType::LinesAdjacency, VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY },
		{ PrimitiveType::LineStripAdjacency, VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY },
		{ PrimitiveType::Triangles, VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST },
		{ PrimitiveType::TriangleStrips, VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP },
	};
	ET_ASSERT(lookup.count(type) > 0);
	return lookup.at(type);
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
		ET_FAIL("Invalid CullMode");
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
		ET_FAIL("Invalid IndexArrayFormat");
	}
}

VkSamplerAddressMode textureWrapToSamplerAddressMode(TextureWrap wrap)
{
	static std::map<TextureWrap, VkSamplerAddressMode> values =
	{
		{ TextureWrap::Repeat, VK_SAMPLER_ADDRESS_MODE_REPEAT },
		{ TextureWrap::MirrorRepeat, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
		{ TextureWrap::ClampToEdge, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE },
	};
	ET_ASSERT(values.count(wrap) > 0);
	return values.at(wrap);
}

VkFilter textureFiltrationValue(TextureFiltration flt)
{
	static std::map<TextureFiltration, VkFilter> values =
	{
		{ TextureFiltration::Nearest, VK_FILTER_NEAREST },
		{ TextureFiltration::Linear, VK_FILTER_LINEAR },
	};
	ET_ASSERT(values.count(flt) > 0);
	return values.at(flt);
}

VkSamplerMipmapMode textureFiltrationValueToSamplerMipMapMode(TextureFiltration flt)
{
	static std::map<TextureFiltration, VkSamplerMipmapMode> values =
	{
		{ TextureFiltration::Nearest, VK_SAMPLER_MIPMAP_MODE_NEAREST },
		{ TextureFiltration::Linear, VK_SAMPLER_MIPMAP_MODE_LINEAR },
	};
	ET_ASSERT(values.count(flt) > 0);
	return values.at(flt);
}

VkAttachmentLoadOp frameBufferOperationToLoadOperation(FramebufferOperation val)
{
	static std::map<FramebufferOperation, VkAttachmentLoadOp> values =
	{
		{ FramebufferOperation::Clear, VK_ATTACHMENT_LOAD_OP_CLEAR },
		{ FramebufferOperation::Load, VK_ATTACHMENT_LOAD_OP_LOAD },
		{ FramebufferOperation::DontCare, VK_ATTACHMENT_LOAD_OP_DONT_CARE },
	};
	ET_ASSERT(values.count(val) > 0);
	return values.at(val);
}

VkAttachmentStoreOp frameBufferOperationToStoreOperation(FramebufferOperation val)
{
	static std::map<FramebufferOperation, VkAttachmentStoreOp> values =
	{
		{ FramebufferOperation::Store, VK_ATTACHMENT_STORE_OP_STORE },
		{ FramebufferOperation::DontCare, VK_ATTACHMENT_STORE_OP_DONT_CARE },
	};
	ET_ASSERT(values.count(val) > 0);
	return values.at(val);
}

namespace gl
{
DataType dataTypeFromGLType(int glType)
{
	static std::map<int, DataType> validTypes = {
		{ GL_FLOAT, DataType::Float },
		{ GL_FLOAT_VEC2, DataType::Vec2 },
		{ GL_FLOAT_VEC3, DataType::Vec3 },
		{ GL_FLOAT_VEC4, DataType::Vec4 },
		{ GL_INT, DataType::Int },
		{ GL_INT_VEC2, DataType::IntVec2 },
		{ GL_INT_VEC3, DataType::IntVec3 },
		{ GL_INT_VEC4, DataType::IntVec4 },
		{ GL_FLOAT_MAT3, DataType::Mat3 },
		{ GL_FLOAT_MAT4, DataType::Mat4 },
	};
	ET_ASSERT(validTypes.count(glType) > 0);
	return validTypes.at(glType);
}

bool isSamplerType(int glType)
{
	static std::set<int> validTypes = {
		GL_SAMPLER_1D,
		GL_SAMPLER_2D,
		GL_SAMPLER_3D,
		GL_SAMPLER_CUBE,
		GL_SAMPLER_1D_SHADOW,
		GL_SAMPLER_2D_SHADOW,
		GL_SAMPLER_1D_ARRAY,
		GL_SAMPLER_2D_ARRAY,
		GL_SAMPLER_1D_ARRAY_SHADOW,
		GL_SAMPLER_2D_ARRAY_SHADOW,
		GL_SAMPLER_CUBE_SHADOW,
		GL_SAMPLER_2D_RECT,
		GL_SAMPLER_2D_RECT_SHADOW,
	};
	return validTypes.count(glType) > 0;
}
}

}

}
