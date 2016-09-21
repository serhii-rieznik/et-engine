/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_WIN)

#include <et/rendering/dx12/dx12_renderer.h>
#include <et/rendering/vulkan/vulkan_renderer.h>
#include <et/rendering/base/helpers.h>

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

	application().initContext();

	if (app->parameters().renderingAPI == RenderingAPI::Vulkan)
	{
		_renderer = VulkanRenderer::Pointer::create(this);
	}
		else if (app->parameters().renderingAPI == RenderingAPI::DX12)
	{
		_renderer = DX12Renderer::Pointer::create(this);
	}
	else 
	{
		ET_FAIL("Invalid or unsupported rendering api provided");
	}
	_renderer->init(inParams);

	HWND wnd = reinterpret_cast<HWND>(application().context().objects[0]);
	RECT clientRect = { };
	GetClientRect(wnd, &clientRect);
	_size.x = clientRect.right - clientRect.left;
	_size.y = clientRect.bottom - clientRect.top;

	ShowWindow(wnd, SW_SHOW);
	SetForegroundWindow(wnd);
	SetFocus(wnd);

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

	application().freeContext();

	// TODO: DO STUFF

	if (application().parameters().shouldPreserveRenderContext)
	{
		popRenderingContext();
	}
	ET_PIMPL_FINALIZE(RenderContext)
}

void RenderContext::init()
{
	renderhelper::init(this);
}

void RenderContext::shutdown()
{

}

bool RenderContext::beginRender()
{
	if (application().parameters().shouldPreserveRenderContext)
	{
		pushAndActivateRenderingContext();
	}

	_renderer->begin();

	return true;
}

void RenderContext::endRender()
{
	_renderer->present();

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
