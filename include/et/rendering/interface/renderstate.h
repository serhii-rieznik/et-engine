/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#define ET_EXPOSE_OLD_RENDER_STATE 0

#include <et/rendering/rendering.h>

namespace et
{
	class RenderContext;
	class RenderState : public Shared
	{
	public:
		ET_DECLARE_POINTER(RenderState);

	public:
		virtual ~RenderState() = default;

		virtual void setBlendState(const BlendState&) = 0;
		virtual void setDepthState(const DepthState&) = 0;
		virtual void setCullMode(const CullMode&) = 0;
	};
}
