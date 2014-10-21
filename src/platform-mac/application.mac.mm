/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
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
	
	delegate()->setApplicationParameters(_parameters);
	
	RenderContextParameters parameters;
	delegate()->setRenderContextParameters(parameters);
	
#if !defined(ET_CONSOLE_APPLICATION)
	_renderContext = new RenderContext(parameters, this);
	
	NSMenu* mainMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] init];
	NSMenuItem* applicationMenuItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] init];
	[mainMenu addItem:applicationMenuItem];
	[[NSApplication sharedApplication] setMainMenu:mainMenu];
	
	NSString* quitTitle = [NSString stringWithFormat:@"Quit %@", [[NSProcessInfo processInfo] processName]];
	NSMenuItem* quitItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:quitTitle
		action:@selector(terminate:) keyEquivalent:@"q"];
	
	NSMenu* applicationMenu = [[NSMenu allocWithZone:[NSMenu menuZone]] init];
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

void Application::quit(int)
{
#if !defined(ET_CONSOLE_APPLICATION)
	[[NSApplication sharedApplication] terminate:nil];
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
	delete _delegate, _delegate = nullptr;
	delete _renderContext, _renderContext = nullptr;
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
