/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/platform-apple/apple.h>
#include <et/app/applicationnotifier.h>

#if (ET_PLATFORM_MAC)

#include <AppKit/NSApplication.h>
#include <AppKit/NSMenu.h>
#include <AppKit/NSWindow.h>

using namespace et;

/*
 * etApplicationDelegate Interface
 */

@interface etApplicationDelegate : NSObject<NSApplicationDelegate>
{
	ApplicationNotifier _notifier;
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
	
#if !defined(ET_CONSOLE_APPLICATION)
	_renderContext = sharedObjectFactory().createObject<RenderContext>(parameters, this);
	
	NSMenu* mainMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] init];
	NSMenuItem* applicationMenuItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] init];
	[mainMenu addItem:applicationMenuItem];
	[[NSApplication sharedApplication] setMainMenu:mainMenu];
	
	NSMenu* applicationMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] init];
	
	if ((application().parameters().windowStyle & WindowStyle_Sizable) == WindowStyle_Sizable)
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
#endif
	
	_runLoop.updateTime(_lastQueuedTimeMSec);
	enterRunLoop();
}

void Application::quit(int code)
{
#if defined(ET_CONSOLE_APPLICATION)
	_running = false;
	_exitCode = code;
#else
	[[NSApplication sharedApplication] performSelectorOnMainThread:@selector(terminate:) withObject:nil waitUntilDone:NO];
	(void)code;
#endif
}

void Application::setTitle(const std::string &s)
{
#if !defined(ET_CONSOLE_APPLICATION)
	NSWindow* mainWindow = [[[NSApplication sharedApplication] windows] objectAtIndex:0];
	[mainWindow setTitle:[NSString stringWithUTF8String:s.c_str()]];
#endif
}

void Application::alert(const std::string&, const std::string&, AlertType)
{	
}

void Application::platformInit()
{
	_env.updateDocumentsFolder(_identifier);
}

int Application::platformRun(int, char*[])
{
#if defined(ET_CONSOLE_APPLICATION)
	
	loaded();
	
#else
	
	@autoreleasepool
	{
		etApplicationDelegate* delegate = ET_OBJC_AUTORELEASE([[etApplicationDelegate alloc] init]);
		[[NSApplication sharedApplication] setDelegate:delegate];
		[[NSApplication sharedApplication] run];
	}
	
#endif
	
	return _exitCode;
}

void Application::platformFinalize()
{
	sharedObjectFactory().deleteObject(_delegate);
	sharedObjectFactory().deleteObject(_renderContext);
	
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
#if !defined(ET_CONSOLE_APPLICATION)
	[[NSApplication sharedApplication] requestUserAttention:NSCriticalRequest];
#endif
}

/*
 *
 * etApplicationDelegate implementation
 *
 */

@implementation etApplicationDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    (void)notification;
	_notifier.notifyLoaded();
}

- (void)applicationWillBecomeActive:(NSNotification *)notification
{
    (void)notification;
	_notifier.notifyActivated();
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
    (void)notification;
	_notifier.notifyDeactivated();
}

- (void)applicationDidHide:(NSNotification *)notification
{
    (void)notification;
	_notifier.notifyDeactivated();
}

- (void)applicationDidUnhide:(NSNotification *)notification
{
    (void)notification;
	_notifier.notifyActivated();
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    (void)notification;
	_notifier.notifyTerminated();
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    (void)sender;
	return YES;
}

@end

#endif // ET_PLATFORM_MAC
