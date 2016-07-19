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
#include <et/rendering/renderhelper.h>
#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/opengl/openglrenderer.h>
#include <et/input/input.h>
#include <et/app/application.h>

using namespace et;

class et::RenderContextPrivate
{
public:
	RenderContextPrivate(RenderContext*, RenderContextParameters&, ApplicationParameters&);
    ~RenderContextPrivate();
	
	int displayLinkSynchronized();
	
	void run();
	void resize(const vec2i&);
	void performUpdateAndRender();
	void stop();
		
	bool canPerformOperations()
		{ return !firstSync && (displayLink != nil); }
	
public:
    RenderContext* owner = nullptr;
    CGLPixelFormatObj pixelFormat = nullptr;
	CGLContextObj glContext = nullptr;
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
	_renderer = OpenGLRenderer::Pointer::create(this);

	_renderState.setRenderContext(this);
	_renderState.setDefaultFramebuffer(_framebufferFactory->createFramebufferWrapper(0));
	_renderState.setMainViewportSize(_renderState.viewportSize());

	renderhelper::init(this);
	
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
	renderhelper::release();
}

bool RenderContext::beginRender()
{
    if (_private->canPerformOperations() == false)
        return false;

    OpenGLCounters::reset();
    
    CGLLockContext(_private->glContext);
    CGLSetCurrentContext(_private->glContext);
 
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
    
    ET_ASSERT(CGLGetCurrentContext() == _private->glContext);
    CGLFlushDrawable(_private->glContext);
    CGLUnlockContext(_private->glContext);
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
    application().initContext();
    const auto& ctx = application().context();
    
    bool msaaEnabled = params.multisamplingQuality != MultisamplingQuality::None;
    CGLPixelFormatAttribute attribs[128] =
    {
        kCGLPFADoubleBuffer,
        kCGLPFAColorSize, CGLPixelFormatAttribute(24),
        kCGLPFAAlphaSize, CGLPixelFormatAttribute(8),
        kCGLPFADepthSize, CGLPixelFormatAttribute(32),
        kCGLPFABackingStore, CGLPixelFormatAttribute(1),
        kCGLPFAAccelerated,
        kCGLPFAOpenGLProfile, CGLPixelFormatAttribute(kCGLOGLPVersion_GL4_Core),
    };
    
    size_t msaaFirstEntry = 0;
    while (attribs[++msaaFirstEntry]);
    
    if (msaaEnabled)
    {
        attribs[msaaFirstEntry+0] = kCGLPFAMultisample;
        attribs[msaaFirstEntry+1] = kCGLPFASampleBuffers;
        attribs[msaaFirstEntry+2] = CGLPixelFormatAttribute(1);
        attribs[msaaFirstEntry+3] = kCGLPFASamples;
        attribs[msaaFirstEntry+4] = CGLPixelFormatAttribute(32);
    }
    
    GLint numPixelFormats = 0;
    auto err = CGLChoosePixelFormat(attribs, &pixelFormat, &numPixelFormats);
    ET_ASSERT(err == kCGLNoError);
    
    err = CGLCreateContext(pixelFormat, nullptr, &glContext);
    ET_ASSERT(err == kCGLNoError);
    
    CGLSetCurrentContext(glContext);
    GLint swap = static_cast<GLint>(params.swapInterval);
    CGLSetParameter(glContext, kCGLCPSwapInterval, &swap);
    
    NSWindow* mainWindow = (NSWindow*)CFBridgingRelease(ctx.pointers[0]);
    
    NSOpenGLView* openGlView = (NSOpenGLView*)CFBridgingRelease(ctx.pointers[2]);
    [openGlView setOpenGLContext:[[NSOpenGLContext alloc] initWithCGLContextObj:glContext]];
    
	[mainWindow makeKeyAndOrderFront:[NSApplication sharedApplication]];
	[mainWindow orderFrontRegardless];
	
	[[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}

RenderContextPrivate::~RenderContextPrivate()
{
    CGLDestroyContext(glContext);
    CGLDestroyPixelFormat(pixelFormat);
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
		CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, glContext, pixelFormat);
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
