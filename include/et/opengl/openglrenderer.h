/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/renderinterface.h>

namespace et
{
	class RenderContext;
	class OpenGLRenderer : public RenderInterface
	{
	public:
		ET_DECLARE_POINTER(OpenGLRenderer)

	public:
		OpenGLRenderer(RenderContext*);

		RenderPass::Pointer allocateRenderPass(const RenderPass::ConstructionInfo&) override;
		void submitRenderPass(RenderPass::Pointer) override;

		/*
		 * Low level stuff
		 */
		void drawIndexedPrimitive(PrimitiveType, IndexArrayFormat, uint32_t first, uint32_t count) override;
	};
}