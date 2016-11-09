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
public:
    LRESULT mainWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

RenderContext::RenderContext(const RenderContextParameters& inParams, Application* app) : 
	_params(inParams)
{
	ET_PIMPL_INIT(RenderContext)

	if (app->parameters().shouldPreserveRenderContext)
	{
		pushAndActivateRenderingContext();
	}

	application().initContext();

    HWND mainWindow = reinterpret_cast<HWND>(application().context().objects[0]);
    SetWindowLongPtr(mainWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(_private));

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

    RECT clientRect = { };
	GetClientRect(mainWindow, &clientRect);
	_size.x = clientRect.right - clientRect.left;
	_size.y = clientRect.bottom - clientRect.top;

	ShowWindow(mainWindow, SW_SHOW);
	SetForegroundWindow(mainWindow);
	SetFocus(mainWindow);

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

LRESULT RenderContextPrivate::mainWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CLOSE:
        case WM_QUIT:
        {
            application().quit(0);
            return 0;
        }
    }
    return DefWindowProc(wnd, msg, wParam, lParam);
}

/*
* Windows procedure
*/
LRESULT WINAPI mainWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LONG_PTR context = GetWindowLongPtr(wnd, GWLP_USERDATA);
    return (context != 0) ? 
        reinterpret_cast<RenderContextPrivate*>(context)->mainWindowProc(wnd, msg, wParam, lParam) : 
        DefWindowProc(wnd, msg, wParam, lParam);
}

}

#endif // ET_PLATFORM_WIN
