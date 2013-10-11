/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <AppKit/NSApplication.h>
#include <AppKit/NSMenu.h>
#include <AppKit/NSWindow.h>
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
	_runLoop.update(_lastQueuedTimeMSec);
	
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
	
#if (!ET_OBJC_ARC_ENABLED)
	[mainMenu autorelease];
	[applicationMenuItem autorelease];
	[quitItem autorelease];
	[applicationMenu autorelease];
#endif
		
	enterRunLoop();
}

void Application::enterRunLoop()
{
	_running = true;
	
	delegate()->applicationDidLoad(_renderContext);
	setActive(true);
	
	_renderContext->init();
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

int Application::platformRun()
{
	@autoreleasepool
	{
		etApplicationDelegate* delegate = [[etApplicationDelegate alloc] init];
		
		[[NSApplication sharedApplication] setDelegate:delegate];
		[[NSApplication sharedApplication] run];
		
#if (!ET_OBJC_ARC_ENABLED)
		[delegate release];
#endif
		
		return 0;
	}
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
