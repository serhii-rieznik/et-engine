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

class VulkanSamplerPrivate : public VulkanNativeSampler
{
public:
	VulkanSamplerPrivate(VulkanState& v) :
		vulkan(v)
	{
	}

	VulkanState& vulkan;
};

VulkanSampler::VulkanSampler(VulkanState& vulkan, const Sampler::Description& desc)
{
	ET_PIMPL_INIT(VulkanSampler, vulkan);

	VkSamplerCreateInfo info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	info.addressModeU = vulkan::textureWrapToSamplerAddressMode(desc.wrapU);
	info.addressModeV = vulkan::textureWrapToSamplerAddressMode(desc.wrapV);
	info.addressModeW = vulkan::textureWrapToSamplerAddressMode(desc.wrapW);
	info.minFilter = vulkan::textureFiltrationValue(desc.minFilter);
	info.magFilter = vulkan::textureFiltrationValue(desc.magFilter);
	info.mipmapMode = vulkan::textureFiltrationValueToSamplerMipMapMode(desc.mipFilter);
	info.maxLod = std::numeric_limits<float>::max();
	info.anisotropyEnable = desc.maxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
	info.maxAnisotropy = desc.maxAnisotropy;
	
	vkCreateSampler(vulkan.device, &info, nullptr, &_private->sampler);
}

VulkanSampler::~VulkanSampler()
{
	vkDestroySampler(_private->vulkan.device, _private->sampler, nullptr);
	ET_PIMPL_FINALIZE(VulkanSampler);
}

const VulkanNativeSampler& VulkanSampler::nativeSampler() const
{
	return *(_private);
}

}
