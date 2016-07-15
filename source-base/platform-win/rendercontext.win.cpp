/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_WIN)

#include <et/app/application.h>
#include <Windows.h>

namespace et
{

class RenderContextPrivate
{

};

RenderContext::RenderContext(const RenderContextParameters& inParams, Application* app) : 
	_params(inParams), _app(app)
{
	ET_PIMPL_INIT(RenderContext)

	if (app->parameters().shouldPreserveRenderContext)
	{
		pushAndActivateRenderingContext();
	}

	// TODO: DO STUFF

	if (app->parameters().shouldPreserveRenderContext)
	{
		popRenderingContext();
	}
}

RenderContext::~RenderContext()
{
	if (application().parameters().shouldPreserveRenderContext)
	{
		pushAndActivateRenderingContext();
	}

	// TODO: DO STUFF

	if (application().parameters().shouldPreserveRenderContext)
	{
		popRenderingContext();
	}
	ET_PIMPL_FINALIZE(RenderContext)
}

void RenderContext::init()
{
	RECT r = { };
	HWND mainWindow = reinterpret_cast<HWND>(application().context().pointers[0]);

	GetClientRect(mainWindow, &r);

	_renderState.setMainViewportSize(vec2i(r.right - r.left, r.bottom - r.top));
	_fpsTimer.expired.connect(this, &RenderContext::onFPSTimerExpired);
	_fpsTimer.start(mainTimerPool().ptr(), 1.0f, -1);
}

bool RenderContext::valid()
{
	return _private != nullptr;
}

bool RenderContext::beginRender()
{
	if (application().parameters().shouldPreserveRenderContext)
	{
		pushAndActivateRenderingContext();
	}

	_renderState.bindDefaultFramebuffer();
	return true;
}

void RenderContext::endRender()
{
	if (application().parameters().shouldPreserveRenderContext)
	{
		popRenderingContext();
	}
}

void RenderContext::performResizing(const vec2i&)
{
	ET_FAIL("Not implemented");
}

void RenderContext::pushRenderingContext()
{
}

bool RenderContext::activateRenderingContext()
{
	return false;
}

bool RenderContext::pushAndActivateRenderingContext()
{
	return false;
}

void RenderContext::popRenderingContext()
{
}

}

#endif // ET_PLATFORM_WIN
