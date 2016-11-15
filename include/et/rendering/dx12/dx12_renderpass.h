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
	class DX12RenderPass : public RenderPass
	{
	public:
		ET_DECLARE_POINTER(DX12RenderPass);

	public:
		DX12RenderPass(const RenderPass::ConstructionInfo&);
		
		void begin() override;
		void pushRenderBatch(const RenderBatch::Pointer&) override;
		void end() override;
	};
}
