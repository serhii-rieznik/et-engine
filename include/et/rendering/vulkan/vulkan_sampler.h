/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/sampler.h>

namespace et
{
class VulkanState;
class VulkanNativeSampler;
class VulkanSamplerPrivate;
class VulkanSampler : public Sampler
{
public:
	ET_DECLARE_POINTER(VulkanSampler);

public:
	VulkanSampler(VulkanState& vulkan, const Description&);
	~VulkanSampler();

	const VulkanNativeSampler& nativeSampler() const;

private:
	ET_DECLARE_PIMPL(VulkanSampler, 64);
};
}
