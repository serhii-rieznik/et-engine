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

class VulkanState;
class VulkanRenderer;
class VulkanNativeRenderPass;
class VulkanRenderPassPrivate;
class VulkanRenderPass : public RenderPass
{
public:
	ET_DECLARE_POINTER(VulkanRenderPass);

public:
	VulkanRenderPass(VulkanRenderer*, VulkanState&, const RenderPass::ConstructionInfo&);
	~VulkanRenderPass();

	const VulkanNativeRenderPass& nativeRenderPass() const;

	void pushRenderBatch(RenderBatch::Pointer) override;
	
	void endRenderPass();
	void submit();

private:
	ET_DECLARE_PIMPL(VulkanRenderPass, 128);
};
}
