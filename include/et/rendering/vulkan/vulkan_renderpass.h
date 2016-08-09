/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/renderpass.h>

namespace et
{
	class VulkanRenderPass : public RenderPass
	{
	public:
		ET_DECLARE_POINTER(VulkanRenderPass);

	public:
		VulkanRenderPass(const RenderPass::ConstructionInfo&);
		void pushRenderBatch(RenderBatch::Pointer) override;
	};
}
