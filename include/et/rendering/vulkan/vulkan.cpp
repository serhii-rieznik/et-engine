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

void VulkanSwapchain::create(VulkanState& vulkan)
{
	VkSurfaceCapabilitiesKHR surfaceCaps = { };
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan.physicalDevice, surface, &surfaceCaps);

	auto presentModes = enumerateVulkanObjects<VkPresentModeKHR>(vulkan, vkGetPhysicalDeviceSurfacePresentModesKHRWrapper);
	extent = surfaceCaps.currentExtent;

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	// TODO : handle v-sync

	uint32_t numImages = surfaceCaps.minImageCount + 1;
	if ((surfaceCaps.maxImageCount > 0) && (numImages > surfaceCaps.maxImageCount))
	{
		numImages = surfaceCaps.maxImageCount;
	}

	VkSurfaceTransformFlagsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	ET_ASSERT(surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR);

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
	swapchainInfo.oldSwapchain = nullptr;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VULKAN_CALL(vkCreateSwapchainKHR(vulkan.device, &swapchainInfo, nullptr, &swapchain));

	images = enumerateVulkanObjects<VkImage>(vulkan, vkGetSwapchainImagesKHRWrapper);
	imageViews.resize(images.size());

	VkImageView* imageViewPtr = imageViews.data();
	for (VkImage image : images)
	{
		VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.image = image;
		viewInfo.format = surfaceFormat.format;
		viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		VULKAN_CALL(vkCreateImageView(vulkan.device, &viewInfo, nullptr, imageViewPtr));
		++imageViewPtr;
	}
}

void VulkanSwapchain::acquireNextImage(VulkanState& vulkan)
{
	VULKAN_CALL(vkAcquireNextImageKHR(vulkan.device, swapchain, UINT64_MAX, 
		vulkan.semaphores.imageAvailable, nullptr, &currentImageIndex));
}

uint32_t getVulkanMemoryType(VulkanState& vulkan, uint32_t typeFilter, VkMemoryPropertyFlags properties) 
{
	static bool propertiesRetreived = false;
	static VkPhysicalDeviceMemoryProperties memProperties = { };

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

VulkanNativeBuffer::VulkanNativeBuffer(VulkanState& vulkan, uint32_t size, uint32_t usage) : 
	_vulkan(vulkan)
{
	VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	info.size = size;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VULKAN_CALL(vkCreateBuffer(_vulkan.device, &info, nullptr, &_buffer));

	VkMemoryRequirements req = { };
	vkGetBufferMemoryRequirements(_vulkan.device, _buffer, &req);

	VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = req.size;
	allocInfo.memoryTypeIndex = getVulkanMemoryType(_vulkan, 
		req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VULKAN_CALL(vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &_memory));

	VULKAN_CALL(vkBindBufferMemory(vulkan.device, _buffer, _memory, 0));
}

VulkanNativeBuffer::~VulkanNativeBuffer()
{
	vkDestroyBuffer(_vulkan.device, _buffer, nullptr);
	vkFreeMemory(_vulkan.device, _memory, nullptr);
}

void* VulkanNativeBuffer::map(uint32_t offset, uint32_t size)
{
	void* pointer = nullptr;
	{
		ET_ASSERT(_mapped == false);
		VULKAN_CALL(vkMapMemory(_vulkan.device, _memory, offset, size, 0, &pointer));
		_mapped = true;
	}
	return pointer;
}

void VulkanNativeBuffer::unmap()
{
	ET_ASSERT(_mapped);
	vkUnmapMemory(_vulkan.device, _memory);
	_mapped = false;
}

namespace vulkan
{

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

}

}
