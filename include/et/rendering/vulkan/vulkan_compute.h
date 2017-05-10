/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/compute.h>
#include <et/rendering/base/material.h>

namespace et
{
class VulkanState;
class VulkanNativeCompute;
class VulkanComputePrivate;
class VulkanCompute : public Compute
{
public:
	ET_DECLARE_POINTER(VulkanCompute);

public:
	VulkanCompute(VulkanState& vulkan, const Material::Pointer&);
	~VulkanCompute();

	const VulkanNativeCompute& nativeCompute() const;

private:
	ET_DECLARE_PIMPL(VulkanCompute, 64);
};
}
