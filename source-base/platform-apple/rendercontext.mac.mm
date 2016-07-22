/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_MAC)

#include <AppKit/NSApplication.h>
#include <AppKit/NSOpenGL.h>
#include <AppKit/NSOpenGLView.h>
#include <AppKit/NSWindow.h>
#include <CoreVideo/CVDisplayLink.h>

#include <et/platform/platformtools.h>
#include <et/core/threading.h>
#include <et/rendering/renderhelper.h>
#include <et/opengl/openglrenderer.h>
#include <et/app/application.h>

using namespace et;

class et::RenderContextPrivate
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

RenderContext::RenderContext(const RenderContextParameters& inParams, Application* app)
	: _params(inParams)
	, _app(app)
{
	ET_PIMPL_INIT(RenderContext)

	application().initContext();
	const auto& ctx = application().context();

	_renderer = OpenGLRenderer::Pointer::create(this);
	_renderer->init(_params);

	_textureFactory = TextureFactory::Pointer::create(this);
	_framebufferFactory = FramebufferFactory::Pointer::create(this);
	_materialFactory = MaterialFactory::Pointer::create(this);
	_vertexBufferFactory = VertexBufferFactory::Pointer::create(this);

	_renderState.setRenderContext(this);
	_renderState.setDefaultFramebuffer(_framebufferFactory->createFramebufferWrapper(0));
	_renderState.setMainViewportSize(_renderState.viewportSize());

	renderhelper::init(this);

	NSWindow* mainWindow = (NSWindow*)CFBridgingRelease(ctx.objects[0]);

	NSOpenGLView* openGlView = (NSOpenGLView*)CFBridgingRelease(ctx.objects[2]);
	CGLContextObj glContext = reinterpret_cast<CGLContextObj>(ctx.objects[4]);
	[openGlView setOpenGLContext:[[NSOpenGLContext alloc] initWithCGLContextObj:glContext]];

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
	_renderState.bindDefaultFramebuffer();

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

void RenderContext::performResizing(const vec2i& newSize)
{
	if (_private->initialized)
	{
		_renderState.defaultFramebuffer()->resize(newSize);
	}
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

#endif // ET_PLATFORM_MAC
