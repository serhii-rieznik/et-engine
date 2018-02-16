/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/objectscache.h>
#include <et/rendering/rendercontextparams.h>
#include <et/rendering/interface/renderer.h>

namespace et {

class Application;
class RenderContextPrivate;
class RenderContext
{
public:
	RenderContext();
	~RenderContext();

	void init(const ApplicationParameters& appParams, const RenderContextParameters& rcParams);
	void shutdown();

	RenderInterface::Pointer& renderer() {
		return _renderer;
	}

	bool beginRender();
	void endRender();

	void performResizing(const vec2i&);

private:
	ET_DENY_COPY(RenderContext);
	ET_DENY_MOVE(RenderContext);

private:
	ET_DECLARE_PIMPL(RenderContext, 256);
	RenderInterface::Pointer _renderer;
};

}
