/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_sampler.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanSamplerPrivate
{
public:
	VulkanSamplerPrivate(VulkanState& v) :
		vulkan(v)
	{
	}

	VulkanState& vulkan;
	VulkanNativeSampler sampler;
};

VulkanSampler::VulkanSampler(VulkanState& vulkan, const Sampler::Description& desc)
{
	ET_PIMPL_INIT(VulkanSampler, vulkan);

	VkSamplerCreateInfo info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	// TODO : fill structure
	vkCreateSampler(vulkan.device, &info, nullptr, &_private->sampler.sampler);
}

VulkanSampler::~VulkanSampler()
{
	ET_PIMPL_FINALIZE(VulkanSampler);
}

const VulkanNativeSampler& VulkanSampler::nativeSampler() const
{
	return _private->sampler;
}

}
