/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <mach/mach.h>

#include <et/app/application.h>
#include <et/opengl/opengl.h>
#include <et/platform-ios/applicationdelegate.h>

#if defined(ET_EMBEDDED_APPLICATION)
#	include <et/opengl/openglcaps.h>
#else
#	include <et/platform-ios/openglviewcontroller.h>
#endif

NSString* etKeyboardRequiredNotification = @"etKeyboardRequiredNotification";
NSString* etKeyboardNotRequiredNotification = @"etKeyboardNotRequiredNotification";

using namespace et;

void Application::platformInit()
{
	_env.updateDocumentsFolder(_identifier);
}

void Application::platformFinalize()
{
	delete _delegate, _delegate = nullptr;
	delete _renderContext, _delegate = nullptr;
}

void Application::platformActivate()
{
	etApplicationDelegate* appDelegate = [[UIApplication sharedApplication] delegate];
	[appDelegate beginUpdates];
}

void Application::platformDeactivate()
{
	etApplicationDelegate* appDelegate = [[UIApplication sharedApplication] delegate];
	[appDelegate endUpdates];
}

void Application::platformSuspend()
{
	etApplicationDelegate* appDelegate = [[UIApplication sharedApplication] delegate];
	[appDelegate endUpdates];
}

void Application::platformResume()
{
	etApplicationDelegate* appDelegate = [[UIApplication sharedApplication] delegate];
	[appDelegate beginUpdates];
}

int Application::platformRun(int argc, char* argv[])
{
#if defined(ET_EMBEDDED_APPLICATION)
	
	loaded();
	return 0;
	
#else

    @autoreleasepool
	{
		std::string appName = _launchParameters.front();
		StringDataStorage appNameData(appName.size());
		etCopyMemory(appNameData.data(), appName.c_str(), appName.size());
		@try
		{
			NSString* delegateClass = NSStringFromClass([etApplicationDelegate class]);
			return UIApplicationMain(argc, argv, nil, delegateClass);
		}
		@catch (NSException *exception)
		{
			NSLog(@"%@, %@", exception, [exception callStackSymbols]);
		}
    }
	
#endif	
}

void Application::loaded()
{
	float scale = [[UIScreen mainScreen] scale];
	CGRect winBounds = [[UIScreen mainScreen] bounds];
	
	RenderContextParameters renderContextParams;
	renderContextParams.contextSize.x = static_cast<int>(winBounds.size.width * scale);
	renderContextParams.contextSize.y = static_cast<int>(winBounds.size.height * scale);
	delegate()->setRenderContextParameters(renderContextParams);
	
	_renderContext = new RenderContext(renderContextParams, this);
	_renderingContextHandle = _renderContext->renderingContextHandle();
	_runLoop.update(_lastQueuedTimeMSec);
	_renderContext->init();
    
    enterRunLoop();
}

void Application::enterRunLoop()
{
	_running = true;
	
	delegate()->applicationDidLoad(_renderContext);
	setActive(true);
}

void Application::quit(int exitCode)
{
	_running = false;
	
#if defined(ET_EMBEDDED_APPLICATION)
	
	setActive(false);
	terminated();
	
	delete _delegate;
	delete _renderContext;
	
	_delegate = 0;
	_renderContext = 0;
	
#else	
	
	exit(exitCode);
	
#endif
}

void Application::alert(const std::string& title, const std::string& message, AlertType)
{
	NSString* nsTitle = [NSString stringWithCString:title.c_str() encoding:NSASCIIStringEncoding];
	NSString* nsMessage = [NSString stringWithCString:message.c_str() encoding:NSASCIIStringEncoding];
	
	UIAlertView* alert = [[UIAlertView alloc] initWithTitle:nsTitle message:nsMessage delegate:nil
		cancelButtonTitle:@"Close" otherButtonTitles:nil];
	
	[alert show];
	
#if (!ET_OBJC_ARC_ENABLED)
	[alert release];
#endif
}

size_t Application::memoryUsage() const
{
	struct task_basic_info info = { };
	mach_msg_type_number_t size = sizeof(info);
	kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
	return (kerr == KERN_SUCCESS) ? info.resident_size : 0;
}

void Application::setTitle(const std::string&)
{
	
}
