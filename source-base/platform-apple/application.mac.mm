/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/platform-apple/apple.h>

#if (ET_PLATFORM_MAC)

#include <AppKit/NSApplication.h>
#include <AppKit/NSMenu.h>
#include <AppKit/NSWindow.h>
#include <AppKit/NSAlert.h>

#include <et/core/base64.h>
#include <et/core/json.h>
#include <et/app/applicationnotifier.h>

using namespace et;

/*
 * etApplicationDelegate Interface
 */

@interface etApplicationDelegate : NSObject<NSApplicationDelegate>
{
	NSMutableArray* _scheduledURLs;
	ApplicationNotifier _notifier;
	BOOL _launchingFinished;
}

@end

/*
 * Application implementation
 */
void Application::loaded()
{
	_lastQueuedTimeMSec = queryContiniousTimeInMilliSeconds();
	_runLoop.updateTime(_lastQueuedTimeMSec);
		
	RenderContextParameters parameters;
	delegate()->setRenderContextParameters(parameters);
	
	_renderContext = etCreateObject<RenderContext>(parameters, this);
	
	NSMenu* mainMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] init];
	NSMenuItem* applicationMenuItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] init];
	[mainMenu addItem:applicationMenuItem];
	[[NSApplication sharedApplication] setMainMenu:mainMenu];
	
	NSMenu* applicationMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] init];
	
    if (application().parameters().context.style & ContextOptions::Style::Sizable)
	{
		NSMenuItem* fullScreen = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:@""
			action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
		[applicationMenu addItem:fullScreen];
	}
	
	NSString* quitTitle = [NSString stringWithFormat:@"Quit %@", [[NSProcessInfo processInfo] processName]];
	NSMenuItem* quitItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:quitTitle
		action:@selector(terminate:) keyEquivalent:@"q"];
	[applicationMenu addItem:quitItem];
	
	[applicationMenuItem setSubmenu:applicationMenu];
	
	[[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];

	(void)ET_OBJC_AUTORELEASE(mainMenu);
	(void)ET_OBJC_AUTORELEASE(applicationMenuItem);
	(void)ET_OBJC_AUTORELEASE(quitItem);
	(void)ET_OBJC_AUTORELEASE(applicationMenu);
	
	_runLoop.updateTime(_lastQueuedTimeMSec);
	enterRunLoop();
}

Application::~Application()
{
	platformFinalize();
	exitRunLoop();
}

void Application::quit(int code)
{
	[[NSApplication sharedApplication] performSelectorOnMainThread:@selector(terminate:) withObject:nil waitUntilDone:NO];
	(void)code;
}

void Application::setTitle(const std::string &s)
{
	NSString* titleToSet = [NSString stringWithUTF8String:s.c_str()];
	dispatch_async(dispatch_get_main_queue(),
	^{
		NSArray* allWindows = [[NSApplication sharedApplication] windows];
		for (NSWindow* window in allWindows)
			[window setTitle:titleToSet];
	});
}

void Application::platformInit()
{
	_env.updateDocumentsFolder(_identifier);
}

int Application::platformRun(int, char*[])
{
	@autoreleasepool
	{
		etApplicationDelegate* delegate = ET_OBJC_AUTORELEASE([[etApplicationDelegate alloc] init]);
		[[NSApplication sharedApplication] setDelegate:delegate];
		[[NSApplication sharedApplication] run];
	}
	return _exitCode;
}

void Application::platformFinalize()
{
	etDestroyObject(_delegate);
	etDestroyObject(_renderContext);
	
	_renderContext = nullptr;
	_delegate = nullptr;
}

void Application::platformActivate()
{
}

void Application::platformDeactivate()
{
}

void Application::platformSuspend()
{
}

void Application::platformResume()
{
}

void Application::requestUserAttention()
{
	[[NSApplication sharedApplication] requestUserAttention:NSCriticalRequest];
}

void Application::enableRemoteNotifications()
{
	[[NSApplication sharedApplication] registerForRemoteNotificationTypes:NSRemoteNotificationTypeBadge |
		NSRemoteNotificationTypeSound | NSRemoteNotificationTypeAlert];
}

/*
 *
 * etApplicationDelegate implementation
 *
 */

@implementation etApplicationDelegate

- (void)handleURLEventWithURL:(NSString*)url
{
	et::Dictionary systemEvent;
	systemEvent.setStringForKey(kSystemEventType, kSystemEventOpenURL);
	systemEvent.setStringForKey("url", std::string([url UTF8String]));
	et::application().systemEvent.invokeInMainRunLoop(systemEvent);
}

- (void)handleURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
	(void)replyEvent;
	NSString* eventURL = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
	if ([eventURL length] == 0) return;
	
	if (_launchingFinished)
		[self handleURLEventWithURL:eventURL];
	else
		[_scheduledURLs addObject:eventURL];
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{
	(void)notification;
	_launchingFinished = NO;
	
	_scheduledURLs = [[NSMutableArray alloc] init];
	
	[[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
		andSelector:@selector(handleURLEvent:withReplyEvent:)
		forEventClass:kInternetEventClass andEventID:kAEGetURL];

	et::application().systemEvent.invokeInMainRunLoop(Dictionary());
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;
	_notifier.notifyLoaded();
	
	_launchingFinished = YES;
		
	for (NSString* eventURL in _scheduledURLs)
		[self handleURLEventWithURL:eventURL];
	
	NSUserNotification* remoteNotification = [notification.userInfo objectForKeyedSubscript:NSApplicationLaunchUserNotificationKey];
	if (remoteNotification != nil)
	{
		[self application:[NSApplication sharedApplication] didReceiveRemoteNotification:remoteNotification.userInfo];
	}
}

- (void)applicationWillBecomeActive:(NSNotification*)notification
{
    (void)notification;
	_notifier.notifyActivated();
}

- (void)applicationWillResignActive:(NSNotification*)notification
{
    (void)notification;
	_notifier.notifyDeactivated();
}

- (void)applicationDidHide:(NSNotification*)notification
{
    (void)notification;
	_notifier.notifyDeactivated();
}

- (void)applicationDidUnhide:(NSNotification*)notification
{
    (void)notification;
	_notifier.notifyActivated();
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
	(void)notification;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    (void)sender;
	return YES;
}

- (void)application:(NSApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
{
	(void)application;
	BinaryDataStorage dataWrapper(reinterpret_cast<const unsigned char*>([deviceToken bytes]), [deviceToken length]);
	
	Dictionary event;
	event.setStringForKey(kSystemEventType, kSystemEventRemoteNotificationStatusChanged);
	event.setStringForKey("token", base64::encode(dataWrapper));
	
	et::application().systemEvent.invokeInMainRunLoop(event);
}

- (void)application:(NSApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
	(void)application;
	
	Dictionary event;
	event.setStringForKey(kSystemEventType, kSystemEventRemoteNotificationStatusChanged);
	event.setStringForKey("error", std::string([[error localizedDescription] UTF8String]));
	et::application().systemEvent.invokeInMainRunLoop(event);
}

- (void)application:(NSApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo
{
	(void)application;
	
	NSError* error = nil;
	NSData* jsonData = [NSJSONSerialization dataWithJSONObject:userInfo options:NSJSONWritingPrettyPrinted error:&error];
	if (jsonData != nil)
	{
		Dictionary systemEvent;
		systemEvent.setStringForKey(kSystemEventType, kSystemEventRemoteNotification);
		
		VariantClass vc = VariantClass::Invalid;
		auto object = json::deserialize(reinterpret_cast<const char*>([jsonData bytes]), [jsonData length], vc);
		if (vc == VariantClass::Invalid)
		{
			log::error("Unable to get remote notification info.");
			systemEvent.setObjectForKey("info", Dictionary());
		}
		else
		{
			systemEvent.setObjectForKey("notification", object);
		}
		et::application().systemEvent.invokeInMainRunLoop(systemEvent);
	}
	else if (error != nil)
	{
		NSLog(@"Unable to convert remote notification info to JSON: %@", error);
	}
}

@end

#endif // ET_PLATFORM_MAC
