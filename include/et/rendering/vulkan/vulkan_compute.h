/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/compute.h>

namespace et
{
class VulkanState;
class VulkanRenderPass;
class VulkanNativePipeline;
class VulkanComputePrivate;
using VulkanRenderPassPointer = IntrusivePtr<VulkanRenderPass>;

class VulkanCompute : public Compute
{
public:
	ET_DECLARE_POINTER(VulkanCompute);

public:
	VulkanCompute(VulkanState& vulkan, const Material::Pointer&);
	~VulkanCompute();

	void build(const VulkanRenderPassPointer&);

	const VulkanNativePipeline& nativeCompute() const;

private:
	ET_DECLARE_PIMPL(VulkanCompute, 64);
};
}
