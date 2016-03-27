/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_MAC)

#include <AppKit/NSWindow.h>
#include <AppKit/NSWindowController.h>
#include <AppKit/NSAlert.h>
#include <AppKit/NSOpenGL.h>
#include <AppKit/NSOpenGLView.h>
#include <AppKit/NSScreen.h>
#include <AppKit/NSMenu.h>
#include <AppKit/NSTrackingArea.h>
#include <CoreVideo/CVDisplayLink.h>

#include <et/platform/platformtools.h>
#include <et/platform-apple/apple.h>
#include <et/core/threading.h>
#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/input/input.h>
#include <et/app/applicationnotifier.h>

using namespace et;

@interface etWindowController : NSWindowController<NSWindowDelegate>
{
@public
	ApplicationNotifier applicationNotifier;
	RenderContextPrivate* rcPrivate;
}
@end

@interface etOpenGLWindow : NSWindow
{
	NSMutableCharacterSet* allowedCharacters;
}

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag;

@end

@interface etOpenGLView : NSOpenGLView
{
@public
	NSTrackingArea* _trackingArea;
	Input::PointerInputSource pointerInputSource;
	Input::GestureInputSource gestureInputSource;
	ApplicationNotifier applicationNotifier;
	RenderContextPrivate* rcPrivate;
}

@end

class et::RenderContextPrivate
{
public:
	RenderContextPrivate(RenderContext*, RenderContextParameters&, const ApplicationParameters&);
	~RenderContextPrivate();
	
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
	etWindowController* windowController = nil;
	etOpenGLWindow* mainWindow = nil;
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
	const ApplicationParameters& appParams)
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
	
	NSUInteger windowMask = NSBorderlessWindowMask | NSClosableWindowMask;
	
	if (appParams.windowSize != WindowSize::Fullscreen)
	{
		if (appParams.windowStyle & WindowStyle_Caption)
			windowMask |= NSTitledWindowMask | NSMiniaturizableWindowMask;
		
		if (appParams.windowStyle & WindowStyle_Sizable)
			windowMask |= NSResizableWindowMask;
	}
	else
	{
		windowMask |= NSFullScreenWindowMask;
	}
	
	NSScreen* mainScreen = [NSScreen mainScreen];
	NSRect visibleRect = [mainScreen visibleFrame];
	
	NSRect contentRect = { };
	if (appParams.windowSize == WindowSize::FillWorkarea)
	{
		contentRect = [NSWindow contentRectForFrameRect:visibleRect styleMask:windowMask];
	}
	else if (appParams.windowSize == WindowSize::Fullscreen)
	{
		[[NSApplication sharedApplication] setPresentationOptions:NSApplicationPresentationAutoHideDock |
			NSApplicationPresentationAutoHideMenuBar | NSApplicationPresentationFullScreen];
		
		windowMask |= NSFullScreenWindowMask;
		contentRect = [mainScreen frame];
	}
	else
	{
		contentRect = NSMakeRect(0.5f * (visibleRect.size.width - params.contextSize.x),
			visibleRect.origin.y + 0.5f * (visibleRect.size.height - params.contextSize.y),
			params.contextSize.x, params.contextSize.y);
	}
	
	contentRect.origin.x = std::floor(contentRect.origin.x);
	contentRect.origin.y = std::floor(contentRect.origin.y);
	contentRect.size.width = std::floor(contentRect.size.width);
	contentRect.size.height = std::floor(contentRect.size.height);
	
	mainWindow = [[etOpenGLWindow alloc] initWithContentRect:contentRect
		styleMask:windowMask backing:NSBackingStoreBuffered defer:NO];
	
	windowController = [[etWindowController alloc] initWithWindow:mainWindow];
	windowController->rcPrivate = this;
	
	if (appParams.keepWindowAspectOnResize)
		[mainWindow setContentAspectRatio:contentRect.size];
	
	[mainWindow setMinSize:NSMakeSize(static_cast<float>(params.contextMinimumSize.x),
		static_cast<float>(params.contextMinimumSize.y))];

#if (ET_OBJC_ARC_ENABLED)
	CFRetain((__bridge CFTypeRef)mainWindow);
#endif
		
	params.contextSize = vec2i(static_cast<int>(contentRect.size.width),
		static_cast<int>(contentRect.size.height));
	
	etOpenGLView* openGlView = [[etOpenGLView alloc] init];
	[openGlView setAcceptsTouchEvents:YES];
    [openGlView setWantsBestResolutionOpenGLSurface:params.enableHighResolutionContext ? YES : NO];
	openGlView->rcPrivate = this;

	if (appParams.windowStyle & WindowStyle_Sizable)
		[mainWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
	
	[mainWindow setDelegate:windowController];
	[mainWindow setOpaque:YES];
	[mainWindow setContentView:openGlView];
	
	[openGlView setOpenGLContext:[[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil]];
	cOpenGLContext = [[openGlView openGLContext] CGLContextObj];
	
	const int swap = static_cast<int>(params.swapInterval);
	
	CGLSetCurrentContext(cOpenGLContext);
	CGLSetParameter(cOpenGLContext, kCGLCPSwapInterval, &swap);
	
	[mainWindow makeKeyAndOrderFront:[NSApplication sharedApplication]];
	[mainWindow orderFrontRegardless];
	
	[[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}

RenderContextPrivate::~RenderContextPrivate()
{
	ET_OBJC_RELEASE(windowController);
	mainWindow = nil;
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
	if (windowController->applicationNotifier.shouldPerformRendering())
	{
		CGLLockContext(cOpenGLContext);
		CGLSetCurrentContext(cOpenGLContext);
		
		windowController->applicationNotifier.notifyUpdate();
		
		CGLFlushDrawable(cOpenGLContext);
		CGLUnlockContext(cOpenGLContext);
	}
}

int RenderContextPrivate::displayLinkSynchronized()
{
	@synchronized(windowController)
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
		
		auto& notifier = windowController->applicationNotifier;
		notifier.accessRenderContext()->renderState().defaultFramebuffer()->resize(newSize);
		notifier.notifyResize(newSize);
		
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

/*
 * OpenGL View implementation
 */
@implementation etOpenGLView

- (BOOL)canBecomeKeyView
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (PointerInputInfo)mousePointerInfo:(NSEvent*)theEvent withType:(PointerType)type
{
	NSRect ownFrame = [self convertRectToBacking:self.frame];
	NSPoint nativePoint = [self convertPointToBacking:[theEvent locationInWindow]];
	
	vec2 p(static_cast<float>(nativePoint.x),
		static_cast<float>(ownFrame.size.height - nativePoint.y));
	
	vec2 np(2.0f * p.x / static_cast<float>(ownFrame.size.width) - 1.0f,
		1.0f - 2.0f * p.y / static_cast<float>(ownFrame.size.height));

	return PointerInputInfo(type, p, np, vec2(0.0f), static_cast<size_t>([theEvent eventNumber]),
		mainTimerPool()->actualTime(), PointerOrigin_Any);
}

- (void)mouseDown:(NSEvent*)theEvent
{
	pointerInputSource.pointerPressed([self mousePointerInfo:theEvent withType:PointerType_General]);
}

- (void)mouseMoved:(NSEvent*)theEvent
{
	pointerInputSource.pointerMoved([self mousePointerInfo:theEvent withType:PointerType_None]);
}

- (void)mouseDragged:(NSEvent*)theEvent
{
	pointerInputSource.pointerMoved([self mousePointerInfo:theEvent withType:PointerType_General]);
}

- (void)mouseUp:(NSEvent*)theEvent
{
	pointerInputSource.pointerReleased([self mousePointerInfo:theEvent withType:PointerType_General]);
}

- (void)rightMouseDown:(NSEvent*)theEvent
{
	pointerInputSource.pointerPressed([self mousePointerInfo:theEvent withType:PointerType_RightButton]);
}

- (void)rightMouseDragged:(NSEvent*)theEvent
{
	pointerInputSource.pointerMoved([self mousePointerInfo:theEvent withType:PointerType_RightButton]);
}

- (void)rightMouseUp:(NSEvent*)theEvent
{
	pointerInputSource.pointerReleased([self mousePointerInfo:theEvent withType:PointerType_RightButton]);
}

- (void)scrollWheel:(NSEvent*)theEvent
{
	NSRect ownFrame = [self convertRectToBacking:self.frame];
	NSPoint nativePoint = [self convertPointToBacking:[theEvent locationInWindow]];

	vec2 p(static_cast<float>(nativePoint.x),
		static_cast<float>(ownFrame.size.height - nativePoint.y));
	
	vec2 np(2.0f * p.x / static_cast<float>(ownFrame.size.width) - 1.0f,
		1.0f - 2.0f * p.y / static_cast<float>(ownFrame.size.height));
	
	vec2 scroll(static_cast<float>([theEvent deltaX] / ownFrame.size.width),
		static_cast<float>([theEvent deltaY] / ownFrame.size.height));

	PointerOrigin origin = (([theEvent momentumPhase] != NSEventPhaseNone) ||
		([theEvent phase] != NSEventPhaseNone)) ? PointerOrigin_Trackpad : PointerOrigin_Mouse;

	pointerInputSource.pointerScrolled(PointerInputInfo(PointerType_General, p, np,
		scroll, [theEvent hash], static_cast<float>([theEvent timestamp]), origin));
}

- (void)magnifyWithEvent:(NSEvent*)event
{
	gestureInputSource.gesturePerformed(
		GestureInputInfo(GestureTypeMask_Zoom, static_cast<float>(event.magnification)));
}

- (void)swipeWithEvent:(NSEvent*)event
{
	gestureInputSource.gesturePerformed(GestureInputInfo(GestureTypeMask_Swipe,
		static_cast<float>(event.deltaX), static_cast<float>(event.deltaY)));
}

- (void)rotateWithEvent:(NSEvent*)event
{
	gestureInputSource.gesturePerformed(
		GestureInputInfo(GestureTypeMask_Rotate, event.rotation));
}

- (void)drawRect:(NSRect)dirtyRect
{
	(void)dirtyRect;

	if (rcPrivate->canPerformOperations())
		rcPrivate->performUpdateAndRender();
}

- (void)reshape
{
	[super reshape];
	
	if (_trackingArea)
		[self removeTrackingArea:_trackingArea];
	
	_trackingArea = ET_OBJC_AUTORELEASE([[NSTrackingArea alloc] initWithRect:[self bounds]
		options:NSTrackingMouseMoved | NSTrackingActiveAlways owner:self userInfo:nil]);
	
	[self addTrackingArea:_trackingArea];
	
	rcPrivate->resize([self convertRectToBacking:self.bounds].size);
}

@end

@implementation etOpenGLWindow : NSWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)aStyle
	backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag
{
	self = [super initWithContentRect:contentRect styleMask:aStyle backing:bufferingType defer:flag];
	if (self)
	{
		allowedCharacters = [[NSMutableCharacterSet alloc] init];
		[allowedCharacters formUnionWithCharacterSet:[NSCharacterSet alphanumericCharacterSet]];
		[allowedCharacters formUnionWithCharacterSet:[NSCharacterSet punctuationCharacterSet]];
		[allowedCharacters formUnionWithCharacterSet:[NSCharacterSet symbolCharacterSet]];
		[allowedCharacters formUnionWithCharacterSet:[NSCharacterSet symbolCharacterSet]];
		[allowedCharacters formUnionWithCharacterSet:[NSCharacterSet whitespaceCharacterSet]];
	}
	return self;
}

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

- (BOOL)canBecomeMainWindow
{
	return YES;
}

- (void)keyDown:(NSEvent*)theEvent
{
	Input::KeyboardInputSource().keyPressed(theEvent.keyCode);
	
	NSString* filteredString = [theEvent.characters
		stringByTrimmingCharactersInSet:[allowedCharacters invertedSet]];
	
	if ([filteredString length] > 0)
	{
		std::string cString([filteredString cStringUsingEncoding:NSUTF8StringEncoding]);
		Input::KeyboardInputSource().charactersEntered(cString);
	}
}

- (void)keyUp:(NSEvent*)theEvent
{
	Input::KeyboardInputSource().keyReleased(theEvent.keyCode);
}

- (void)flagsChanged:(NSEvent*)theEvent
{
	if (theEvent.modifierFlags & NSDeviceIndependentModifierFlagsMask)
		Input::KeyboardInputSource().keyPressed(theEvent.keyCode);
	else
		Input::KeyboardInputSource().keyReleased(theEvent.keyCode);
}

@end

@implementation etWindowController

- (NSString*)windowTitleForDocumentDisplayName:(NSString*)displayName
{
	(void)displayName;
	return nil;
}

- (void)windowWillClose:(NSNotification*)notification
{
	(void)notification;
	applicationNotifier.notifyStopped();
	rcPrivate->stop();
	applicationNotifier.notifyTerminated();
}

@end

#endif // ET_PLATFORM_MAC
