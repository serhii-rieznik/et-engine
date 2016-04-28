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
#include <et/platform-apple/apple.h>
#include <et/platform-apple/context_osx.h>
#include <et/core/threading.h>
#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/input/input.h>
#include <et/app/applicationnotifier.h>

using namespace et;

class et::RenderContextPrivate
{
public:
	RenderContextPrivate(RenderContext*, RenderContextParameters&, ApplicationParameters&);
	
	int displayLinkSynchronized();
	
	void run();
	void resize(const NSSize&);
	void performUpdateAndRender();
	void stop();
		
	bool canPerformOperations()
		{ return !firstSync && (displayLink != nil); }
	
public:
	uint64_t frameDuration = 0;
	
private:
	NSOpenGLPixelFormat* pixelFormat = nil;
	CGLContextObj cOpenGLContext = nullptr;
	CVDisplayLinkRef displayLink = nullptr;
	NSSize scheduledSize = { };
	bool firstSync = true;
	bool resizeScheduled = false;
};

RenderContext::RenderContext(const RenderContextParameters& inParams, Application* app) : _params(inParams),
	_app(app), _materialFactory(nullptr), _textureFactory(nullptr), _framebufferFactory(nullptr),
	_vertexBufferFactory(nullptr), _renderer(nullptr)
{
	ET_PIMPL_INIT(RenderContext, this, _params, app->parameters())
	
	OpenGLCapabilities::instance().checkCaps();
		
	_textureFactory = TextureFactory::Pointer::create(this);
	_framebufferFactory = FramebufferFactory::Pointer::create(this);
	_materialFactory = MaterialFactory::Pointer::create(this);
	_vertexBufferFactory = VertexBufferFactory::Pointer::create(this);
	_renderer = Renderer::Pointer::create(this);

	_renderState.setRenderContext(this);
	_renderState.setDefaultFramebuffer(_framebufferFactory->createFramebufferWrapper(0));
	_renderState.setMainViewportSize(_renderState.viewportSize());
	
	ET_CONNECT_EVENT(_fpsTimer.expired, RenderContext::onFPSTimerExpired)
}

RenderContext::~RenderContext()
{
	ET_PIMPL_FINALIZE(RenderContext)
}

void RenderContext::init()
{
	_fpsTimer.start(currentTimerPool(), 1.0f, NotifyTimer::RepeatForever);
	_private->run();
}

size_t RenderContext::renderingContextHandle()
{
	return 0;
}

void RenderContext::beginRender()
{
	checkOpenGLError("RenderContext::beginRender");
	
	OpenGLCounters::reset();
	
	_private->frameDuration = queryCurrentTimeInMicroSeconds();
	_renderState.bindDefaultFramebuffer();
}

void RenderContext::endRender()
{
	checkOpenGLError("RenderContext::endRender");

	++_info.averageFramePerSecond;
	
	_info.averageDIPPerSecond += OpenGLCounters::DIPCounter;
	_info.averagePolygonsPerSecond += OpenGLCounters::primitiveCounter;
	_info.averageFrameTimeInMicroseconds += queryCurrentTimeInMicroSeconds() - _private->frameDuration;
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

/*
 *
 * RenderContextPrivate
 *
 */
CVReturn cvDisplayLinkOutputCallback(CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*,
	CVOptionFlags, CVOptionFlags*, void* displayLinkContext);

RenderContextPrivate::RenderContextPrivate(RenderContext*, RenderContextParameters& params,
	ApplicationParameters& appParams)
{
	bool msaaEnabled = params.multisamplingQuality != MultisamplingQuality::None;
	
	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] =
	{
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 32,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADoubleBuffer,
		(msaaEnabled ? NSOpenGLPFASampleBuffers : 0u), (msaaEnabled ? 1u : 0u),
		0, 0, 0, 0, 0, 0, 0 // space for multisampling and context profile
	};
	
	size_t lastEntry = 0;
	while (pixelFormatAttributes[++lastEntry] != 0);
	
	pixelFormatAttributes[lastEntry++] = NSOpenGLPFAOpenGLProfile;
	pixelFormatAttributes[lastEntry++] = NSOpenGLProfileVersion4_1Core;
	
	size_t antialiasFirstEntry = 0;
	size_t antialiasSamplesEntry = 0;
	
	if (msaaEnabled)
	{
		antialiasFirstEntry = lastEntry;
		pixelFormatAttributes[lastEntry++] = NSOpenGLPFAMultisample;
		pixelFormatAttributes[lastEntry++] = NSOpenGLPFASamples;
		
		pixelFormatAttributes[lastEntry++] =
			(params.multisamplingQuality == MultisamplingQuality::Best) ? 32 : 1;
		
		antialiasSamplesEntry = lastEntry - 1;
	}
	
	pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
	if ((pixelFormat == nil) && (antialiasSamplesEntry != 0))
	{
		while ((pixelFormat == nil) && (pixelFormatAttributes[antialiasSamplesEntry] > 1))
		{
			pixelFormatAttributes[antialiasSamplesEntry] /= 2;
			pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
		}
		
		if (pixelFormat == nil)
		{
			pixelFormatAttributes[antialiasFirstEntry++] = 0;
			pixelFormatAttributes[antialiasFirstEntry++] = 0;
			pixelFormatAttributes[antialiasFirstEntry++] = 0;
			pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
		}
		
		if (pixelFormat == nil)
		{
			alert("Unable to create and initialize rendering context",
				  "Unable to create NSOpenGLPixelFormat object, application will now terminate.",
				  "Terminate", AlertType::Error);
			exit(1);
		}
	}
	(void)ET_OBJC_AUTORELEASE(pixelFormat);
    
    ApplicationContextFactoryOSX f;
    auto ctx = f.createContextWithOptions(appParams.context);
    
    NSWindow* mainWindow = (NSWindow*)CFBridgingRelease(ctx.pointers[0]);
    NSWindowController* mainWindowController = (NSWindowController*)CFBridgingRelease(ctx.pointers[1]);
    NSOpenGLView* openGlView = (NSOpenGLView*)CFBridgingRelease(ctx.pointers[2]);
	
	[openGlView setOpenGLContext:[[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil]];
	cOpenGLContext = [[openGlView openGLContext] CGLContextObj];
	
	const int swap = static_cast<int>(params.swapInterval);
	
	CGLSetCurrentContext(cOpenGLContext);
	CGLSetParameter(cOpenGLContext, kCGLCPSwapInterval, &swap);
	
	[mainWindow makeKeyAndOrderFront:[NSApplication sharedApplication]];
	[mainWindow orderFrontRegardless];
	
	[[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}

void RenderContextPrivate::run()
{
	if (displayLink == nil)
	{
		CVReturn result = CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
		
		if ((result != kCVReturnSuccess) || (displayLink == nullptr))
		{
			alert("Could not create display link.", "Application will now shut down.",
				"Terminate", AlertType::Error);
			exit(1);
		}

		CVDisplayLinkSetOutputCallback(displayLink, cvDisplayLinkOutputCallback, this);
		CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cOpenGLContext, [pixelFormat CGLPixelFormatObj]);
	}
	
	CVDisplayLinkStart(displayLink);
}

void RenderContextPrivate::stop()
{
	CVDisplayLinkStop(displayLink);
	CVDisplayLinkRelease(displayLink);
	displayLink = nil;
}

void RenderContextPrivate::performUpdateAndRender()
{
    ApplicationNotifier an;
    
    if (an.shouldPerformRendering())
	{
		CGLLockContext(cOpenGLContext);
		CGLSetCurrentContext(cOpenGLContext);
		an.notifyUpdate();
		CGLFlushDrawable(cOpenGLContext);
		CGLUnlockContext(cOpenGLContext);
	}
}

int RenderContextPrivate::displayLinkSynchronized()
{
    if (firstSync)
    {
        threading::setMainThreadIdentifier(threading::currentThread());
        registerRunLoop(mainRunLoop());
        firstSync = false;
    }

    if (application().running() && !application().suspended())
    {
        if (resizeScheduled)
            resize(scheduledSize);
        
        performUpdateAndRender();
    }
    
	return kCVReturnSuccess;
}

void RenderContextPrivate::resize(const NSSize& sz)
{
	if (canPerformOperations())
	{
		CGLLockContext(cOpenGLContext);
		CGLSetCurrentContext(cOpenGLContext);
				
		vec2i newSize(static_cast<int>(sz.width), static_cast<int>(sz.height));
		
        ApplicationNotifier an;
		an.accessRenderContext()->renderState().defaultFramebuffer()->resize(newSize);
		an.notifyResize(newSize);
		
		CGLUnlockContext(cOpenGLContext);
		
		resizeScheduled = false;
	}
	else
	{
		scheduledSize = sz;
		resizeScheduled = true;
	}
}

/*
 * Display link callback
 */
CVReturn cvDisplayLinkOutputCallback(CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*,
	CVOptionFlags, CVOptionFlags*, void* displayLinkContext)
{
	@autoreleasepool
	{
		return static_cast<RenderContextPrivate*>(displayLinkContext)->displayLinkSynchronized();
	}
}

#endif // ET_PLATFORM_MAC
