/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>
#include <et/platform-apple/objc.h>
#include <et/platform-apple/context_osx.h>
#include <et/app/application.h>
#include <et/input/input.h>

#include <AppKit/NSWindow.h>
#include <AppKit/NSWindowController.h>
#include <AppKit/NSScreen.h>
#include <AppKit/NSMenu.h>
#include <AppKit/NSView.h>
#include <AppKit/NSTrackingArea.h>
#include <AppKit/NSOpenGLView.h>

@interface etWindowController : NSWindowController<NSWindowDelegate>

@end
    
@interface etWindow : NSWindow
{
    NSMutableCharacterSet* allowedCharacters;
}

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)aStyle
    backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag;

@end

@interface etOpenGLView : NSOpenGLView
{
@public
    NSTrackingArea* _trackingArea;
    et::Input::PointerInputSource pointerInputSource;
    et::Input::GestureInputSource gestureInputSource;
}
@end

@interface etMetalView : NSView

@end


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

- (et::PointerInputInfo)mousePointerInfo:(NSEvent*)theEvent withType:(et::PointerType)type
{
    NSRect ownFrame = [self convertRectToBacking:self.frame];
    NSPoint nativePoint = [self convertPointToBacking:[theEvent locationInWindow]];
    
    et::vec2 p(static_cast<float>(nativePoint.x),
           static_cast<float>(ownFrame.size.height - nativePoint.y));
    
    et::vec2 np(2.0f * p.x / static_cast<float>(ownFrame.size.width) - 1.0f,
            1.0f - 2.0f * p.y / static_cast<float>(ownFrame.size.height));
    
    return et::PointerInputInfo(type, p, np, et::vec2(0.0f), static_cast<size_t>([theEvent eventNumber]),
        et::mainTimerPool()->actualTime(), et::PointerOrigin_Any);
}

- (void)mouseDown:(NSEvent*)theEvent
{
    pointerInputSource.pointerPressed([self mousePointerInfo:theEvent withType:et::PointerType_General]);
}

- (void)mouseMoved:(NSEvent*)theEvent
{
    pointerInputSource.pointerMoved([self mousePointerInfo:theEvent withType:et::PointerType_None]);
}

- (void)mouseDragged:(NSEvent*)theEvent
{
    pointerInputSource.pointerMoved([self mousePointerInfo:theEvent withType:et::PointerType_General]);
}

- (void)mouseUp:(NSEvent*)theEvent
{
    pointerInputSource.pointerReleased([self mousePointerInfo:theEvent withType:et::PointerType_General]);
}

- (void)rightMouseDown:(NSEvent*)theEvent
{
    pointerInputSource.pointerPressed([self mousePointerInfo:theEvent withType:et::PointerType_RightButton]);
}

- (void)rightMouseDragged:(NSEvent*)theEvent
{
    pointerInputSource.pointerMoved([self mousePointerInfo:theEvent withType:et::PointerType_RightButton]);
}

- (void)rightMouseUp:(NSEvent*)theEvent
{
    pointerInputSource.pointerReleased([self mousePointerInfo:theEvent withType:et::PointerType_RightButton]);
}

- (void)scrollWheel:(NSEvent*)theEvent
{
    NSRect ownFrame = [self convertRectToBacking:self.frame];
    NSPoint nativePoint = [self convertPointToBacking:[theEvent locationInWindow]];
    
    et::vec2 p(static_cast<float>(nativePoint.x),
           static_cast<float>(ownFrame.size.height - nativePoint.y));
    
    et::vec2 np(2.0f * p.x / static_cast<float>(ownFrame.size.width) - 1.0f,
            1.0f - 2.0f * p.y / static_cast<float>(ownFrame.size.height));
    
    et::vec2 scroll(static_cast<float>([theEvent deltaX] / ownFrame.size.width),
                static_cast<float>([theEvent deltaY] / ownFrame.size.height));
    
    et::PointerOrigin origin = (([theEvent momentumPhase] != NSEventPhaseNone) || ([theEvent phase] != NSEventPhaseNone)) ?
        et::PointerOrigin_Trackpad : et::PointerOrigin_Mouse;
    
    pointerInputSource.pointerScrolled(et::PointerInputInfo(et::PointerType_General, p, np,
        scroll, [theEvent hash], static_cast<float>([theEvent timestamp]), origin));
}

- (void)magnifyWithEvent:(NSEvent*)event
{
    gestureInputSource.gesturePerformed(et::GestureInputInfo(et::GestureTypeMask_Zoom,
        static_cast<float>(event.magnification)));
}

- (void)swipeWithEvent:(NSEvent*)event
{
    gestureInputSource.gesturePerformed(et::GestureInputInfo(et::GestureTypeMask_Swipe,
        static_cast<float>(event.deltaX), static_cast<float>(event.deltaY)));
}

- (void)rotateWithEvent:(NSEvent*)event
{
    gestureInputSource.gesturePerformed(et::GestureInputInfo(et::GestureTypeMask_Rotate, event.rotation));
}

- (void)reshape
{
    if (_trackingArea)
        [self removeTrackingArea:_trackingArea];
    
    _trackingArea = ET_OBJC_AUTORELEASE([[NSTrackingArea alloc] initWithRect:[self bounds]
        options:NSTrackingMouseMoved | NSTrackingActiveAlways owner:self userInfo:nil]);
    
    [self addTrackingArea:_trackingArea];

    auto nativeSize = [self convertRectToBacking:self.bounds].size;
	
	et::vec2i newSize(static_cast<int32_t>(nativeSize.width), static_cast<int32_t>(nativeSize.height));
	et::application().resizeContext(newSize);
}

@end
    
@implementation etWindow : NSWindow

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
    et::Input::KeyboardInputSource().keyPressed(theEvent.keyCode);
    
    NSString* filteredString = [theEvent.characters stringByTrimmingCharactersInSet:[allowedCharacters invertedSet]];
    
    if ([filteredString length] > 0)
    {
        std::string cString([filteredString cStringUsingEncoding:NSUTF8StringEncoding]);
        et::Input::KeyboardInputSource().charactersEntered(cString);
    }
}

- (void)keyUp:(NSEvent*)theEvent
{
    et::Input::KeyboardInputSource().keyReleased(theEvent.keyCode);
}

- (void)flagsChanged:(NSEvent*)theEvent
{
    if (theEvent.modifierFlags & NSDeviceIndependentModifierFlagsMask)
        et::Input::KeyboardInputSource().keyPressed(theEvent.keyCode);
    else
        et::Input::KeyboardInputSource().keyReleased(theEvent.keyCode);
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
    et::application().stop();
}

@end

namespace et
{
    
PlatformDependentContext ApplicationContextFactoryOSX::createContextWithOptions(RenderingAPI api, ContextOptions& options)
{
    NSUInteger windowMask = NSBorderlessWindowMask | NSClosableWindowMask;
    
    if (options.sizeClass != ContextOptions::SizeClass::Fullscreen)
    {
        if (options.style & ContextOptions::Style::Caption)
            windowMask |= NSTitledWindowMask | NSMiniaturizableWindowMask;
        
        if (options.style & ContextOptions::Style::Sizable)
            windowMask |= NSResizableWindowMask;
    }
    else
    {
        windowMask |= NSFullScreenWindowMask;
    }
    
    NSScreen* mainScreen = [NSScreen mainScreen];
    NSRect visibleRect = [mainScreen visibleFrame];
    
    NSRect contentRect = { };
    if (options.sizeClass == ContextOptions::SizeClass::FillWorkarea)
    {
        contentRect = [NSWindow contentRectForFrameRect:visibleRect styleMask:windowMask];
    }
    else if (options.sizeClass == ContextOptions::SizeClass::Fullscreen)
    {
        [[NSApplication sharedApplication] setPresentationOptions:NSApplicationPresentationAutoHideDock |
         NSApplicationPresentationAutoHideMenuBar | NSApplicationPresentationFullScreen];
        
        windowMask |= NSFullScreenWindowMask;
        contentRect = [mainScreen frame];
    }
    else
    {
        contentRect = NSMakeRect
        (
            0.5f * (visibleRect.size.width - options.size.x),
            visibleRect.origin.y + 0.5f * (visibleRect.size.height - options.size.y),
            options.size.x,
            options.size.y
         );
    }
    
    contentRect.origin.x = std::floor(contentRect.origin.x);
    contentRect.origin.y = std::floor(contentRect.origin.y);
    contentRect.size.width = std::floor(contentRect.size.width);
    contentRect.size.height = std::floor(contentRect.size.height);
    
    etWindow* mainWindow = [[etWindow alloc] initWithContentRect:contentRect
        styleMask:windowMask backing:NSBackingStoreBuffered defer:NO];
	CFBridgingRetain(mainWindow);

    etWindowController* windowController = [[etWindowController alloc] initWithWindow:mainWindow];
	CFBridgingRetain(windowController);

    if (options.keepAspectOnResize)
        [mainWindow setContentAspectRatio:contentRect.size];
    
    [mainWindow setMinSize:NSMakeSize(static_cast<float>(options.minimumSize.x),
        static_cast<float>(options.minimumSize.y))];

    options.size = vec2i(static_cast<int32_t>(contentRect.size.width),
        static_cast<int32_t>(contentRect.size.height));

	NSView* view = nil;

	if (api == RenderingAPI::OpenGL)
	{
		etOpenGLView* openGlView = [[etOpenGLView alloc] init];
		[openGlView setWantsBestResolutionOpenGLSurface:options.supportsHighResolution ? YES : NO];
		view = openGlView;
	}
	else if (api == RenderingAPI::Metal)
	{
		view = [[NSView alloc] init];
	}

	[view setAcceptsTouchEvents:YES];
	CFBridgingRetain(view);

    if (options.style & ContextOptions::Style::Sizable)
	{
        [mainWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
	}


	[mainWindow setContentView:view];
    [mainWindow setDelegate:windowController];
    [mainWindow setOpaque:YES];

    PlatformDependentContext result;
    result.objects[0] = (__bridge void*)(mainWindow);
    result.objects[1] = (__bridge void*)(windowController);
    result.objects[2] = (__bridge void*)(view);
    return result;
}
 
void ApplicationContextFactoryOSX::destroyContext(PlatformDependentContext context)
{
	CFBridgingRelease(context.objects[0]);
	CFBridgingRelease(context.objects[1]);
	CFBridgingRelease(context.objects[2]);
}
    
}
