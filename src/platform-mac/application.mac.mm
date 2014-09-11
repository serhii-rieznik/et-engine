/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <AppKit/NSApplication.h>
#include <AppKit/NSMenu.h>
#include <AppKit/NSWindow.h>

#include <et/platform-apple/apple.h>
#include <et/app/applicationnotifier.h>

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
	
	(void)ET_OBJC_AUTORELEASE(mainMenu);
	(void)ET_OBJC_AUTORELEASE(applicationMenuItem);
	(void)ET_OBJC_AUTORELEASE(quitItem);
	(void)ET_OBJC_AUTORELEASE(applicationMenu);
			
	_runLoop.updateTime(_lastQueuedTimeMSec);
	enterRunLoop();
}

void Application::quit(int)
{
	[[NSApplication sharedApplication] terminate:nil];
}

void Application::setTitle(const std::string &s)
{
	NSWindow* mainWindow = [[[NSApplication sharedApplication] windows] objectAtIndex:0];
	[mainWindow setTitle:[NSString stringWithUTF8String:s.c_str()]];
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
	@autoreleasepool
	{
		etApplicationDelegate* delegate = ET_OBJC_AUTORELEASE([[etApplicationDelegate alloc] init]);
		[[NSApplication sharedApplication] setDelegate:delegate];
		[[NSApplication sharedApplication] run];
	}
	
	return 0;
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
	[NSApp requestUserAttention:NSCriticalRequest];
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

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    (void)sender;
	return YES;
}

@end
