/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/base64.h>
#include <et/app/application.h>
#include <et/app/applicationnotifier.h>
#include <et/threading/threading.h>
#include <et/threading/mutex.h>

#if (ET_PLATFORM_IOS)

#include <QuartzCore/QuartzCore.h>

#if defined(ET_SUPPORT_FACEBOOK_SDK)
#	include <FacebookSDK/FacebookSDK.h>
#endif

#if defined(ET_SUPPORT_GOOGLE_PLUS)
#	include <GooglePlus/GooglePlus.h>
#endif

#include <et/platform-apple/apple.h>
#include <et/platform-ios/applicationdelegate.h>
#include <et/platform-ios/openglviewcontroller.h>

using namespace et;

@interface etApplicationDelegate()
{
	et::ApplicationNotifier _notifier;
	
	UIWindow* _window;
	NSThread* _renderThread;
	CADisplayLink* _displayLink;
	
	BOOL _shouldUnlockRenderLock;
}

@end

extern etOpenGLViewController* sharedOpenGLViewController;

@implementation etApplicationDelegate

@synthesize window = _window;

- (BOOL)application:(UIApplication*)anApplication didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
	(void)anApplication;
	(void)launchOptions;

#if !defined(ET_EMBEDDED_APPLICATION)
	@synchronized(self)
	{
		_window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
		
		_notifier.notifyLoaded();
		
		[_window setRootViewController:sharedOpenGLViewController];
		[_window makeKeyAndVisible];
	}
#endif
	
    return YES;
}

- (void)applicationWillResignActive:(UIApplication*)application
{
	[self endUpdates];
	
	(void)application;
	
#if !defined(ET_EMBEDDED_APPLICATION)
	@synchronized(sharedOpenGLViewController.context)
	{
		[sharedOpenGLViewController beginRender];
		_notifier.notifyDeactivated();
	}
#endif
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
	(void)application;
	
#if !defined(ET_EMBEDDED_APPLICATION)
	@synchronized(sharedOpenGLViewController.context)
	{
		[sharedOpenGLViewController beginRender];
		_notifier.notifyActivated();
	}
#endif
}

- (void)applicationWillTerminate:(UIApplication*)application
{
	(void)application;
	et::application().quit(0);
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
	(void)application;
	_notifier.notifySuspended();
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
	(void)application;
	_notifier.notifyResumed();
}

- (BOOL)updating
{
#if defined(ET_EMBEDDED_APPLICATION)
	return YES;
#else
	return ![sharedOpenGLViewController suspended];
#endif
}

- (void)tick
{
	if (sharedOpenGLViewController.suspended) return;
	
#if !defined(ET_EMBEDDED_APPLICATION)
	@synchronized(sharedOpenGLViewController.context)
	{
		if (_notifier.shouldPerformRendering())
		{
			[sharedOpenGLViewController beginRender];
			_notifier.notifyIdle();
			[sharedOpenGLViewController endRender];
		}
	}
#endif
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
#if !defined(ET_EMBEDDED_APPLICATION)
		@synchronized(sharedOpenGLViewController.context)
		{
			[sharedOpenGLViewController setSuspended:NO];
		}
#endif
	}
}

- (void)endUpdates
{
#if !defined(ET_EMBEDDED_APPLICATION)
	@synchronized(sharedOpenGLViewController.context)
	{
		[sharedOpenGLViewController setSuspended:YES];
	}
#endif
}

- (void)application:(UIApplication*)application didRegisterUserNotificationSettings:(UIUserNotificationSettings*)notificationSettings
{
	[application registerForRemoteNotifications];
}

- (void)application:(UIApplication*)application handleActionWithIdentifier:(NSString*)identifier
	forRemoteNotification:(NSDictionary*)userInfo completionHandler:(void(^)())completionHandler
{
	
}

- (void)application:(UIApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken
{
	BinaryDataStorage dataWrapper(reinterpret_cast<const unsigned char*>([deviceToken bytes]), [deviceToken length]);
	
	Dictionary event;
	event.setStringForKey(kSystemEventType, kSystemEventRemoteNotificationStatusChanged);
	event.setStringForKey("token", base64::encode(dataWrapper));
	
	et::application().systemEvent.invokeInMainRunLoop(event);
}

- (void)application:(UIApplication*)application didFailToRegisterForRemoteNotificationsWithError:(NSError*)error
{
	Dictionary event;
	
	event.setStringForKey(kSystemEventType, kSystemEventRemoteNotificationStatusChanged);
	event.setStringForKey("error", std::string([[error localizedDescription] UTF8String]));
	
	et::application().systemEvent.invokeInMainRunLoop(event);
}

- (NSUInteger)application:(UIApplication*)application supportedInterfaceOrientationsForWindow:(UIWindow*)window
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
		processed = [GPPURLHandler handleURL:url sourceApplication:sourceApplication
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

#endif // ET_PLATFORM_IOS
