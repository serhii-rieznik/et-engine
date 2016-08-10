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

void vkGetPhysicalDeviceSurfaceFormatsKHRWrapper(const VulkanState& state, uint32_t* count, VkSurfaceFormatKHR* formats)
{
	vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, state.swapchain.surface, count, formats);
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

void vkGetPhysicalDeviceSurfacePresentModesKHRWrapper(const VulkanState& state, uint32_t* count, VkPresentModeKHR* modes)
{
	vkGetPhysicalDeviceSurfacePresentModesKHR(state.physicalDevice, state.swapchain.surface, count, modes);
}

void vkGetSwapchainImagesKHRWrapper(const VulkanState& state, uint32_t* count, VkImage* images)
{
	vkGetSwapchainImagesKHR(state.device, state.swapchain.swapchain, count, images);
}

void VulkanSwapchain::create(VulkanState& vulkan)
{
	VkSurfaceCapabilitiesKHR surfaceCaps = { };
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan.physicalDevice, surface, &surfaceCaps);

	auto presentModes = enumerateVulkanObjects<VkPresentModeKHR>(vulkan, vkGetPhysicalDeviceSurfacePresentModesKHRWrapper);
	VkExtent2D swapchainExtent = surfaceCaps.currentExtent;

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
	swapchainInfo.imageExtent = swapchainExtent;
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

}
