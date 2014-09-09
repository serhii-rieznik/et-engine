/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <AppKit/NSWindow.h>
#include <AppKit/NSWindowController.h>
#include <AppKit/NSAlert.h>
#include <AppKit/NSOpenGL.h>
#include <AppKit/NSOpenGLView.h>
#include <AppKit/NSScreen.h>
#include <AppKit/NSMenu.h>
#include <AppKit/NSTrackingArea.h>
#include <CoreVideo/CVDisplayLink.h>

#include <et/platform-apple/objc.h>
#include <et/opengl/openglcaps.h>
#include <et/input/input.h>
#include <et/app/applicationnotifier.h>
#include <et/threading/threading.h>
#include <et/rendering/rendercontext.h>

using namespace et;

@interface etWindowDelegate : NSObject<NSWindowDelegate>
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
	
private:
	etWindowDelegate* windowDelegate = nil;
	etOpenGLWindow* mainWindow = nil;
	
	NSOpenGLPixelFormat* pixelFormat = nil;
	NSOpenGLContext* openGlContext = nil;
	CVDisplayLinkRef displayLink = nullptr;
	CGLContextObj cglObject = nullptr;
	NSSize scheduledSize = { };
	
	bool firstSync = true;
	bool resizeScheduled = false;
	
public:
	uint64_t frameDuration = 0;
};

RenderContext::RenderContext(const RenderContextParameters& inParams, Application* app) : _params(inParams),
	_app(app), _programFactory(nullptr), _textureFactory(nullptr), _framebufferFactory(nullptr),
	_vertexBufferFactory(nullptr), _renderer(nullptr), _screenScaleFactor(1)
{
	_private = new RenderContextPrivate(this, _params, app->parameters());
	
	openGLCapabilites().checkCaps();
	updateScreenScale(_params.contextSize);
	
	_renderState.setRenderContext(this);
	_renderState.setMainViewportSize(_params.contextSize);
	
	_textureFactory = TextureFactory::Pointer(new TextureFactory(this));
	_framebufferFactory = FramebufferFactory::Pointer(new FramebufferFactory(this));
	_programFactory = ProgramFactory::Pointer(new ProgramFactory(this));
	_vertexBufferFactory = VertexBufferFactory::Pointer(new VertexBufferFactory(this));

	_renderState.setDefaultFramebuffer(_framebufferFactory->createFramebufferWrapper(0));
	_renderer = Renderer::Pointer::create(this);
	
	ET_CONNECT_EVENT(_fpsTimer.expired, RenderContext::onFPSTimerExpired)
}

RenderContext::~RenderContext()
{
	delete _private;
}

void RenderContext::init()
{
	_fpsTimer.start(mainTimerPool(), 1.0f, NotifyTimer::RepeatForever);
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
	NSOpenGLPixelFormatAttribute pixelFormatAttributes[] =
	{
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 32,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFASampleBuffers, 4,
		0, 0, 0, 0, 0, 0, // space for multisampling and context profile
		0,
	};
	
	size_t lastEntry = 0;
	while (pixelFormatAttributes[++lastEntry] != 0);
	
	size_t profileEntry = 0;
	if (params.openGLForwardContext)
	{
		pixelFormatAttributes[lastEntry++] = NSOpenGLPFAOpenGLProfile;
		
		pixelFormatAttributes[lastEntry++] = (params.openGLProfile == OpenGLProfile_Core) ?
			NSOpenGLProfileVersion3_2Core : NSOpenGLProfileVersionLegacy;
		
		profileEntry = lastEntry - 1;
	}
	
	size_t antialiasFirstEntry = 0;
	size_t antialiasSamplesEntry = 0;
	if (params.multisamplingQuality == MultisamplingQuality_Best)
	{
		antialiasFirstEntry = lastEntry;
		pixelFormatAttributes[lastEntry++] = NSOpenGLPFAMultisample;
		pixelFormatAttributes[lastEntry++] = NSOpenGLPFASamples;
		pixelFormatAttributes[lastEntry++] = 16;
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
			if (profileEntry > 0)
			{
				pixelFormatAttributes[profileEntry] = NSOpenGLProfileVersionLegacy;
				pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelFormatAttributes];
				
				if (pixelFormat == nil)
				{
					[[NSAlert alertWithMessageText:@"Unable to init OpenGL context" defaultButton:@"Close"
						alternateButton:nil otherButton:nil informativeTextWithFormat:
						@"Unable to create NSOpenGLPixelFormat object, even without antialiasing and with legacy profile."] runModal];
					exit(1);
				}
			}
			else
			{
				[[NSAlert alertWithMessageText:@"Unable to init OpenGL context" defaultButton:@"Close"
					alternateButton:nil otherButton:nil informativeTextWithFormat:
					@"Unable to create NSOpenGLPixelFormat object, even without antialiasing."] runModal];
				exit(1);
			}
		}
	}
	
	(void)ET_OBJC_AUTORELEASE(pixelFormat);
	
	NSUInteger windowMask = NSBorderlessWindowMask | NSClosableWindowMask;
	
	if (appParams.windowSize != WindowSize_Fullscreen)
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
	if (appParams.windowSize == WindowSize_FillWorkarea)
	{
		contentRect = [NSWindow contentRectForFrameRect:visibleRect styleMask:windowMask];
	}
	else if (appParams.windowSize == WindowSize_Fullscreen)
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
	
	if (appParams.keepWindowAspectOnResize)
		[mainWindow setContentAspectRatio:contentRect.size];

#if (ET_OBJC_ARC_ENABLED)
	CFRetain((__bridge CFTypeRef)mainWindow);
#endif
		
	params.contextSize = vec2i(static_cast<int>(contentRect.size.width),
		static_cast<int>(contentRect.size.height));
	
	windowDelegate = [[etWindowDelegate alloc] init];
	windowDelegate->rcPrivate = this;

	etOpenGLView* openGlView = [[etOpenGLView alloc] init];
	[openGlView setWantsBestResolutionOpenGLSurface:YES];
	openGlView->rcPrivate = this;
	
	[mainWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
	[mainWindow setDelegate:windowDelegate];
	[mainWindow setOpaque:YES];
	[mainWindow setContentView:openGlView];
	
	openGlContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
	[openGlContext makeCurrentContext];
	
	[openGlView setOpenGLContext:openGlContext];
		
	cglObject = static_cast<CGLContextObj>([openGlContext CGLContextObj]);
	
	const int swap = static_cast<int>(params.swapInterval);
	CGLSetParameter(cglObject, kCGLCPSwapInterval, &swap);
	
	[mainWindow makeKeyAndOrderFront:[NSApplication sharedApplication]];
	[mainWindow orderFrontRegardless];
	
	[[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}

RenderContextPrivate::~RenderContextPrivate()
{
	ET_OBJC_RELEASE(openGlContext);
	ET_OBJC_RELEASE(windowDelegate);
	mainWindow = nil;
}

void RenderContextPrivate::run()
{
	if (displayLink == nil)
	{
		CVReturn result = CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
		
		if ((result != kCVReturnSuccess) || (displayLink == nullptr))
		{
			[[NSAlert alertWithMessageText:@"Something went wrong, could not create display link."
				defaultButton:@"Ok" alternateButton:nil otherButton:nil
				informativeTextWithFormat:@"Return code: %d, Application will now shut down.", result] runModal];
			exit(1);
			return;
		}

		CVDisplayLinkSetOutputCallback(displayLink, cvDisplayLinkOutputCallback, this);
		
		CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglObject,
			static_cast<CGLPixelFormatObj>([pixelFormat CGLPixelFormatObj]));
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
	[openGlContext makeCurrentContext];
	CGLLockContext(cglObject);
	
	Threading::setRenderingThread(Threading::currentThread());
	
	windowDelegate->applicationNotifier.notifyIdle();
	
	CGLFlushDrawable(cglObject);
	CGLUnlockContext(cglObject);
}

int RenderContextPrivate::displayLinkSynchronized()
{
	if (firstSync)
	{
		Threading::setMainThread(Threading::currentThread());
		firstSync = false;
		
		if (resizeScheduled)
			resize(scheduledSize);
	}
	
	if (application().running() && !application().suspended())
		performUpdateAndRender();

	return kCVReturnSuccess;
}

void RenderContextPrivate::resize(const NSSize& sz)
{
	if (canPerformOperations())
	{
		[openGlContext makeCurrentContext];
		CGLLockContext(cglObject);
		
		vec2i newSize(static_cast<int>(sz.width), static_cast<int>(sz.height));
		
		auto& notifier = windowDelegate->applicationNotifier;
		notifier.accessRenderContext()->renderState().defaultFramebuffer()->resize(newSize);
		notifier.notifyResize(newSize);
		
		CGLUnlockContext(cglObject);
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

- (PointerInputInfo)mousePointerInfo:(NSEvent*)theEvent withType:(PointerType)type;
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

- (void)mouseDown:(NSEvent *)theEvent
{
	pointerInputSource.pointerPressed([self mousePointerInfo:theEvent withType:PointerType_General]);
}

- (void)mouseMoved:(NSEvent *)theEvent
{
	pointerInputSource.pointerMoved([self mousePointerInfo:theEvent withType:PointerType_None]);
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	pointerInputSource.pointerMoved([self mousePointerInfo:theEvent withType:PointerType_General]);
}

- (void)mouseUp:(NSEvent *)theEvent
{
	pointerInputSource.pointerReleased([self mousePointerInfo:theEvent withType:PointerType_General]);
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	pointerInputSource.pointerPressed([self mousePointerInfo:theEvent withType:PointerType_RightButton]);
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
	pointerInputSource.pointerMoved([self mousePointerInfo:theEvent withType:PointerType_RightButton]);
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
	pointerInputSource.pointerReleased([self mousePointerInfo:theEvent withType:PointerType_RightButton]);
}

- (void)scrollWheel:(NSEvent *)theEvent
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

- (void)magnifyWithEvent:(NSEvent *)event
{
	gestureInputSource.gesturePerformed(
		GestureInputInfo(GestureTypeMask_Zoom, static_cast<float>(event.magnification)));
}

- (void)swipeWithEvent:(NSEvent *)event
{
	gestureInputSource.gesturePerformed(GestureInputInfo(GestureTypeMask_Swipe,
		static_cast<float>(event.deltaX), static_cast<float>(event.deltaY)));
}

- (void)rotateWithEvent:(NSEvent *)event
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

@end

@implementation etWindowDelegate

- (void)windowWillClose:(NSNotification *)notification
{
	(void)notification;
	applicationNotifier.notifyStopped();
	rcPrivate->stop();
}

@end
