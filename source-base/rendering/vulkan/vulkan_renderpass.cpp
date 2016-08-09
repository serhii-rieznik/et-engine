/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_renderpass.h>

namespace et
{

VulkanRenderPass::VulkanRenderPass(const RenderPass::ConstructionInfo& info) 
	: RenderPass(info)
{
}

void VulkanRenderPass::pushRenderBatch(RenderBatch::Pointer)
{
}

}
