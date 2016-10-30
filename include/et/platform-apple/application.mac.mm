/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/platform-apple/apple.h>

#if (ET_PLATFORM_MAC)

#include <et/core/base64.h>
#include <et/core/json.h>
#include <et/platform-apple/context_osx.h>
#include <et/app/application.h>
#include <et/rendering/rendercontext.h>

@interface etApplicationDelegate : NSObject<NSApplicationDelegate>
{
	NSMutableArray* _scheduledURLs;
	BOOL _launchingFinished;
}

- (void)onQuit:(id)sender;

@end

namespace et
{

/*
 * Application implementation
 */
void Application::initContext()
{
    _context = ApplicationContextFactoryOSX().createContextWithOptions(_parameters.renderingAPI, _parameters.context);
    
    NSMenu* mainMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] init];
    NSMenuItem* applicationMenuItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] init];
    [mainMenu addItem:applicationMenuItem];
    [[NSApplication sharedApplication] setMainMenu:mainMenu];
    
    NSMenu* applicationMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] init];
    
    if (_parameters.context.style & ContextOptions::Style::Sizable)
    {
        NSMenuItem* fullScreen = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:@""
            action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
        [applicationMenu addItem:fullScreen];
    }
    
    NSString* quitTitle = [NSString stringWithFormat:@"Quit %@", [[NSProcessInfo processInfo] processName]];
    NSMenuItem* quitItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:quitTitle
        action:@selector(onQuit:) keyEquivalent:@"q"];
    [applicationMenu addItem:quitItem];
    
    [applicationMenuItem setSubmenu:applicationMenu];
    
    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    (void)ET_OBJC_AUTORELEASE(mainMenu);
    (void)ET_OBJC_AUTORELEASE(applicationMenuItem);
    (void)ET_OBJC_AUTORELEASE(quitItem);
    (void)ET_OBJC_AUTORELEASE(applicationMenu);
}

void Application::freeContext()
{
    ApplicationContextFactoryOSX().destroyContext(_context);
}

void Application::load()
{
	_lastQueuedTimeMSec = queryContiniousTimeInMilliSeconds();
	_runLoop.updateTime(_lastQueuedTimeMSec);
		
	RenderContextParameters parameters;
	delegate()->setRenderContextParameters(parameters);
	
	_renderContext = etCreateObject<RenderContext>(parameters, this);
	_standardPathResolver.setRenderContext(_renderContext);

	_runLoop.updateTime(_lastQueuedTimeMSec);
	enterRunLoop();
}

Application::~Application()
{
	platformFinalize();
    freeContext();
}

void Application::quit(int code)
{
	stop();

	exitRunLoop();
	
	_renderContext->shutdown();
	_backgroundThread.stop();
	_backgroundThread.join();

	[[NSApplication sharedApplication] performSelectorOnMainThread:@selector(terminate:) withObject:nil waitUntilDone:YES];
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

} // namespace et

/*
 *
 * etApplicationDelegate implementation
 *
 */
@implementation etApplicationDelegate

- (void)handleURLEventWithURL:(NSString*)url
{
	et::Dictionary systemEvent;
	systemEvent.setStringForKey(et::kSystemEventType, et::kSystemEventOpenURL);
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

	et::application().systemEvent.invokeInMainRunLoop(et::Dictionary());
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;
    et::application().load();
	
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
    et::application().setActive(true);
}

- (void)applicationWillResignActive:(NSNotification*)notification
{
    (void)notification;
    et::application().setActive(false);
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

	et::BinaryDataStorage dataWrapper
	(
		reinterpret_cast<const unsigned char*>([deviceToken bytes]),
		static_cast<uint32_t>([deviceToken length])
	 );

	et::Dictionary event;
	event.setStringForKey(et::kSystemEventType, et::kSystemEventRemoteNotificationStatusChanged);
	event.setStringForKey("token", et::base64::encode(dataWrapper));
	
	et::application().systemEvent.invokeInMainRunLoop(event);
}

- (void)application:(NSApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
	(void)application;
	
	et::Dictionary event;
	event.setStringForKey(et::kSystemEventType, et::kSystemEventRemoteNotificationStatusChanged);
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
		et::Dictionary systemEvent;
		systemEvent.setStringForKey(et::kSystemEventType, et::kSystemEventRemoteNotification);
		
		et::VariantClass vc = et::VariantClass::Invalid;
		auto object = et::json::deserialize(reinterpret_cast<const char*>([jsonData bytes]), [jsonData length], vc);
		if (vc == et::VariantClass::Invalid)
		{
			et::log::error("Unable to get remote notification info.");
			systemEvent.setObjectForKey("info", et::Dictionary());
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

- (void)onQuit:(id)sender
{
	et::application().quit(0);
}

@end

#endif // ET_PLATFORM_MAC
