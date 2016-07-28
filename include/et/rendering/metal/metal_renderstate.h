/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/renderstate.h>

namespace et
{
	class MetalRenderState : public RenderState
	{
	public:
		ET_DECLARE_POINTER(MetalRenderState);

	public:
		void setBlendState(const BlendState&) override;
		void setDepthState(const DepthState&) override;
		void setCullMode(const CullMode&) override;
	};
}
