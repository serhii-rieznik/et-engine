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
	class OpenGLRenderPass : public RenderPass
	{
	public:
		ET_DECLARE_POINTER(OpenGLRenderPass);

	public:
		OpenGLRenderPass(const RenderPass::ConstructionInfo&);
		~OpenGLRenderPass();
		
		void pushRenderBatch(RenderBatch::Pointer) override;

		Vector<RenderBatch::Pointer> mutableRenderBatches()
			{ return _renderBatches; }

		const Vector<RenderBatch::Pointer>& renderBatches() const
			{ return _renderBatches; }

	private:
		Vector<RenderBatch::Pointer> _renderBatches;
	};
}
