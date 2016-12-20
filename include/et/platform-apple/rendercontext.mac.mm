/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_MAC)

#import <CoreVideo/CVDisplayLink.h>

#include <et/core/threading.h>
#include <et/rendering/base/helpers.h>
#include <et/rendering/metal/metal.h>
#include <et/rendering/metal/metal_renderer.h>
#include <et/app/application.h>

namespace et
{

class RenderContextPrivate
{
public:
	CVDisplayLinkRef displayLink = nullptr;
	bool initialized = false;
};

/*
 * Display link callback
 */
CVReturn etDisplayLinkOutputCallback(CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*,
	CVOptionFlags, CVOptionFlags*, void* displayLinkContext);

RenderContext::RenderContext(const RenderContextParameters& inParams, Application* application)
	: _params(inParams)
{
	ET_PIMPL_INIT(RenderContext)

	application->initContext();
	const auto& ctx = application->context();

	if (application->parameters().renderingAPI == RenderingAPI::Metal)
	{
		_renderer = MetalRenderer::Pointer::create(this);
	}
	else
	{
		ET_FAIL("Invalid rendering API specified");
	}

	_renderer->init(_params);

	NSWindow* mainWindow = (__bridge NSWindow*)(ctx.objects[0]);
	NSView* mainView = nil;

	if (application->parameters().renderingAPI == RenderingAPI::Metal)
	{
		mainView = (__bridge NSView*)(ctx.objects[2]);
		mainView.layer = (__bridge CAMetalLayer*)(ctx.objects[4]);
		[mainView setWantsLayer:YES];
	}

	/*
	 * TODO : replace/remove size variable
    NSRect backingRect = [mainView convertRectToBacking:NSMakeRect(0.0f, 0.0f, mainView.bounds.size.width, mainView.bounds.size.height)];
    _size.x = static_cast<int>(backingRect.size.width);
    _size.y = static_cast<int>(backingRect.size.height);
	// */
    
    [mainWindow makeKeyAndOrderFront:[NSApplication sharedApplication]];
	[mainWindow orderFrontRegardless];
	[[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}

RenderContext::~RenderContext()
{
	_renderer->shutdown();
	ET_PIMPL_FINALIZE(RenderContext)
}

void RenderContext::init()
{
	renderhelper::init(this);
	
	if (_private->displayLink == nil)
	{
		CVReturn result = CVDisplayLinkCreateWithActiveCGDisplays(&_private->displayLink);

		if ((result != kCVReturnSuccess) || (_private->displayLink == nullptr))
		{
			alert("Could not create display link.", "Application will now shut down.",
				  "Terminate", AlertType::Error);
			exit(1);
		}

		CVDisplayLinkSetOutputCallback(_private->displayLink, etDisplayLinkOutputCallback, _private);
	}

	CVDisplayLinkStart(_private->displayLink);
}

void RenderContext::shutdown()
{
	CVDisplayLinkStop(_private->displayLink);
	CVDisplayLinkRelease(_private->displayLink);
	_private->displayLink = nil;

	renderhelper::release();
}

bool RenderContext::beginRender()
{
	ET_ASSERT(_private->initialized);
	_renderer->begin();
	return true;
}

void RenderContext::endRender()
{
	_renderer->present();
}

void RenderContext::pushRenderingContext()
{
	ET_FAIL("Not implemented")
}

bool RenderContext::pushAndActivateRenderingContext()
{
	ET_FAIL("Not implemented")
	return true;
}

bool RenderContext::activateRenderingContext()
{
	ET_FAIL("Not implemented")
	return true;
}

void RenderContext::popRenderingContext()
{
	ET_FAIL("Not implemented")
}

void RenderContext::performResizing(const vec2i&)
{
	ET_FAIL("Not implemented");
}

CVReturn etDisplayLinkOutputCallback(CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*,
	CVOptionFlags, CVOptionFlags*, void* displayLinkContext)
{
	@autoreleasepool
	{
		auto& app = application();
		auto rcp = static_cast<RenderContextPrivate*>(displayLinkContext);

		if (rcp->initialized == false)
		{
			threading::setMainThreadIdentifier(threading::currentThread());
			registerRunLoop(mainRunLoop());
			rcp->initialized = true;
		}

		if (app.running() && !app.suspended() && app.shouldPerformRendering())
		{
			app.performUpdateAndRender();
		}

		return kCVReturnSuccess;
	}
}

}
#endif // ET_PLATFORM_MAC
