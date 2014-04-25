/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <QuartzCore/QuartzCore.h>

#if defined(ET_SUPPORT_FACEBOOK_SDK)
#	include <FacebookSDK/FacebookSDK.h>
#endif

#if defined(ET_SUPPORT_GOOGLE_PLUS)
#	include <GooglePlus/GooglePlus.h>
#endif

#include <et/app/application.h>
#include <et/app/applicationnotifier.h>
#include <et/threading/threading.h>
#include <et/platform-ios/applicationdelegate.h>
#include <et/platform-ios/openglviewcontroller.h>

using namespace et;

@interface etApplicationDelegate()
{
	et::ApplicationNotifier _notifier;
	
	UIWindow* _window;
	NSThread* _renderThread;
	CADisplayLink* _displayLink;
}

@end

extern etOpenGLViewController* sharedOpenGLViewController;

@implementation etApplicationDelegate

@synthesize window = _window;

- (BOOL)application:(UIApplication*)anApplication didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	(void)application;
	(void)launchOptions;

	@synchronized(self)
	{
		_window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
		
		_notifier.notifyLoaded();
		
		[_window setRootViewController:sharedOpenGLViewController];
		[_window makeKeyAndVisible];
	}
	
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	(void)application;
	
	[sharedOpenGLViewController setSuspended:YES];
	_notifier.notifyDeactivated();
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	(void)application;
	_notifier.notifyActivated();
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	(void)application;
	et::application().quit(0);
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	(void)application;
	_notifier.notifySuspended();
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	(void)application;
	_notifier.notifyResumed();
}

- (BOOL)updating
{
	return ![sharedOpenGLViewController suspended];
}

- (void)tick
{
	if (!sharedOpenGLViewController.suspended)
	{
		@synchronized(sharedOpenGLViewController.context)
		{
			[sharedOpenGLViewController beginRender];
			_notifier.notifyIdle();
			[sharedOpenGLViewController endRender];
		}
	}
}

- (void)renderThread
{
	@synchronized(self)
	{
		Threading::setMainThread(Threading::currentThread());
		Threading::setRenderingThread(Threading::currentThread());
		
		_displayLink = [[UIScreen mainScreen] displayLinkWithTarget:self selector:@selector(tick)];
		
		const auto& renderContextParameters = _notifier.accessRenderContext()->parameters();
		if (renderContextParameters.swapInterval > 0)
			[_displayLink setFrameInterval:renderContextParameters.swapInterval];
		
		[sharedOpenGLViewController setSuspended:NO];
		[_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
		
		try
		{
			CFRunLoopRun();
		}
		catch (std::exception e)
		{
			log::info("Terminating due to std::exception: %s", e.what());
			abort();
		}
		
		[_displayLink invalidate];
		_displayLink = nil;
		
#if (!ET_OBJC_ARC_ENABLED)
		[_renderThread release];
#endif
		_renderThread = nil;
	}
}

- (void)beginUpdates
{
	if (_renderThread == nil)
	{
		_renderThread = [[NSThread alloc] initWithTarget:self selector:@selector(renderThread) object:nil];
		[_renderThread start];
	}
	else
	{
		[sharedOpenGLViewController setSuspended:NO];
	}
}

- (void)endUpdates
{
	[sharedOpenGLViewController setSuspended:YES];
}

- (NSUInteger)application:(UIApplication*)application supportedInterfaceOrientationsForWindow:(UIWindow *)window
{
	(void)application;
	(void)window;
	
	void* handle = reinterpret_cast<void*>(et::application().renderingContextHandle());
	UIViewController* vc = (__bridge UIViewController*)(handle);
	
	return [vc supportedInterfaceOrientations];
}

- (BOOL)application:(UIApplication*)application openURL:(NSURL*)url
	sourceApplication:(NSString*)sourceApplication annotation:(id)annotation
{
	BOOL processed = NO;
	
#if defined(ET_SUPPORT_FACEBOOK_SDK)
	
    processed = [FBAppCall handleOpenURL:url sourceApplication:sourceApplication];
	
#endif
	
#if defined(ET_SUPPORT_GOOGLE_PLUS)
	if (!processed)
	{
		processed =[GPPURLHandler handleURL:url sourceApplication:sourceApplication
			annotation:annotation];
	}
#endif
	
	(void)sourceApplication;
	(void)application;
	(void)annotation;
	(void)url;
	
	return processed;
}


@end
