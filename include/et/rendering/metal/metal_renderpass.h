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
struct MetalState;
class MetalRenderer;
class MetalRenderPassPrivate;
class MetalRenderPass : public RenderPass
{
public:
	ET_DECLARE_POINTER(MetalRenderPass);

public:
	MetalRenderPass(MetalRenderer*, MetalState&, const RenderPass::ConstructionInfo&);
	~MetalRenderPass();

	void begin() override;
	void pushRenderBatch(RenderBatch::Pointer) override;
	void end() override;

private:
	ET_DECLARE_PIMPL(MetalRenderPass, 256);
};
}
