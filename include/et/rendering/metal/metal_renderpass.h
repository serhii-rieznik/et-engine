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
	class MetalState;
	class MetalRenderPassPrivate;
	class MetalRenderPass : public RenderPass
	{
	public:
		ET_DECLARE_POINTER(MetalRenderPass);

	public:
		MetalRenderPass(MetalState&, const RenderPass::ConstructionInfo&);
		~MetalRenderPass();
		
		void pushRenderBatch(RenderBatch::Pointer) override;

		void endEncoding();

	private:
		ET_DECLARE_PIMPL(MetalRenderPass, 256);
	};
}
