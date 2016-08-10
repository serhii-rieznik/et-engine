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

	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapchain = nullptr;
	VkSurfaceFormatKHR surfaceFormat { };
	Vector<VkImage> images;
	Vector<VkImageView> imageViews;
};

struct VulkanState
{
	VkInstance instance = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkDevice device = nullptr;
	VkCommandPool commandPool = nullptr;
	VkQueue queue = nullptr;

	struct Semaphores
	{
		VkSemaphore present = nullptr;
		VkSemaphore render = nullptr;
	} semaphores;

	VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	VkCommandBuffer setupCommandBuffer = nullptr;
	
	Vector<VkQueueFamilyProperties> queueProperties;
	uint32_t graphicsQueueIndex = static_cast<uint32_t>(-1);
	uint32_t presentQueueIndex = static_cast<uint32_t>(-1);

	VulkanSwapchain swapchain;
};

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

}
