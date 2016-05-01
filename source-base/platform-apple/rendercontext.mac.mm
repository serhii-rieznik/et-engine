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
#include <et/core/threading.h>
#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/input/input.h>
#include <et/app/application.h>

using namespace et;

class et::RenderContextPrivate
{
public:
	RenderContextPrivate(RenderContext*, RenderContextParameters&, ApplicationParameters&);
	
	int displayLinkSynchronized();
	
	void run();
	void resize(const vec2i&);
	void performUpdateAndRender();
	void stop();
		
	bool canPerformOperations()
		{ return !firstSync && (displayLink != nil); }
	
public:
    RenderContext* owner = nullptr;
	NSOpenGLPixelFormat* pixelFormat = nil;
	CGLContextObj cOpenGLContext = nullptr;
	CVDisplayLinkRef displayLink = nullptr;
    uint64_t frameDuration = 0;
    bool firstSync = true;
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

void RenderContext::shutdown()
{
    _private->stop();
}

bool RenderContext::beginRender()
{
    if (_private->canPerformOperations() == false)
        return false;

    OpenGLCounters::reset();
    
    CGLLockContext(_private->cOpenGLContext);
    CGLSetCurrentContext(_private->cOpenGLContext);
 
    _private->frameDuration = queryCurrentTimeInMicroSeconds();
    _renderState.bindDefaultFramebuffer();
    
    return true;
}

void RenderContext::endRender()
{
	checkOpenGLError("RenderContext::endRender");

	++_info.averageFramePerSecond;
	
	_info.averageDIPPerSecond += OpenGLCounters::DIPCounter;
	_info.averagePolygonsPerSecond += OpenGLCounters::primitiveCounter;
	_info.averageFrameTimeInMicroseconds += queryCurrentTimeInMicroSeconds() - _private->frameDuration;
    
    CGLFlushDrawable(_private->cOpenGLContext);
    CGLUnlockContext(_private->cOpenGLContext);
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
    _private->resize(newSize);
}

/*
 *
 * RenderContextPrivate
 *
 */
CVReturn cvDisplayLinkOutputCallback(CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*,
	CVOptionFlags, CVOptionFlags*, void* displayLinkContext);

RenderContextPrivate::RenderContextPrivate(RenderContext* aOwner, RenderContextParameters& params,
    ApplicationParameters& appParams) : owner(aOwner)
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
    
    application().initContext();
    const auto& ctx = application().context();
    
    NSWindow* mainWindow = (NSWindow*)CFBridgingRelease(ctx.pointers[0]);
    // NSWindowController* mainWindowController = (NSWindowController*)CFBridgingRelease(ctx.pointers[1]);
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
    if (application().shouldPerformRendering())
	{
        application().performUpdateAndRender();
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
        performUpdateAndRender();
    }
    
	return kCVReturnSuccess;
}

void RenderContextPrivate::resize(const vec2i& newSize)
{
    if (canPerformOperations())
    {
        owner->renderState().defaultFramebuffer()->resize(newSize);
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
