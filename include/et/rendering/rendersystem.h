/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/renderpass.h>

namespace et
{
	class RenderContext;
	class RenderSystem : public et::Shared
	{
	public:
		ET_DECLARE_POINTER(RenderBatch)
		
		RenderSystem(RenderContext*);
		
		RenderPass::Pointer allocateRenderPass(const RenderPass::ConstructionInfo&);
		void submitRenderPass(RenderPass::Pointer);
		
	private:
		et::RenderContext* _rc = nullptr;
	};
}
