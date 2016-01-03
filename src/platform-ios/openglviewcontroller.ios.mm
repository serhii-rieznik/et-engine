/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/applicationnotifier.h>

#if (ET_PLATFORM_IOS)

#include <QuartzCore/QuartzCore.h>
#include <et/opengl/opengl.h>
#include <et/platform-ios/openglviewcontroller.h>

using namespace et;

extern NSString* etKeyboardRequiredNotification;
extern NSString* etKeyboardNotRequiredNotification;

@interface etOpenGLViewController()
{
	etOpenGLView* _glView;
	et::RenderContextParameters _params;
	et::ApplicationNotifier _notifier;
}

- (BOOL)performInitialization;
- (void)onNotificationRecevied:(NSNotification*)notification;

@end

@implementation etOpenGLViewController

@synthesize context = _context;
@synthesize suspended = _suspended;

- (id)initWithParameters:(RenderContextParameters)params
{
	self = [super init];
	
	if (self)
	{
		_params = params;
		
		BOOL initialized = [self performInitialization];
		
		ET_ASSERT(initialized);
		(void)initialized;
		
		self.view = _glView;
	}
	return self;
}

- (BOOL)performInitialization
{
#if defined(ET_EMBEDDED_APPLICATION)
	_context = [EAGLContext currentContext];
#else

	_glView = [[etOpenGLView alloc] initWithFrame:[[UIScreen mainScreen] bounds] parameters:_params];
	
#	if defined(GL_ES_VERSION_3_0)
		_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
#	endif
	
	if (_context == nil)
		_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	
#endif
	
	if (_context == nil)
	{
		NSLog(@"Failed to create ES context");
		return NO;
	}
	
	if (![EAGLContext setCurrentContext:_context])
	{
#if (!ET_OBJC_ARC_ENABLED)
		[_context release];
#endif
		NSLog(@"Failed to set ES context current");
		return NO;
	}
	
	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(onNotificationRecevied:) name:etKeyboardRequiredNotification object:nil];
	
	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(onNotificationRecevied:) name:etKeyboardNotRequiredNotification object:nil];
	
	return YES;
}

- (void)setRenderContext:(et::RenderContext*)rc
{
	[_glView setRenderContext:rc];
	[_glView setContext:_context];
}

- (void)dealloc
{
    if ([EAGLContext currentContext] == _context)
        [EAGLContext setCurrentContext:nil];
	
#if (!ET_OBJC_ARC_ENABLED)
	[_context release];
    [super dealloc];
#endif
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	[self becomeFirstResponder];
}

- (void)viewDidUnload
{
	[super viewDidUnload];
	[self resignFirstResponder];
	
    if ([EAGLContext currentContext] == _context)
        [EAGLContext setCurrentContext:nil];
	
#if (!ET_OBJC_ARC_ENABLED)
	[_context release];
#endif
	
	_context = nil;	
}

- (NSUInteger)supportedInterfaceOrientations
{
	NSUInteger result = 0;
	
	if (_params.supportedInterfaceOrientations & InterfaceOrientation_Portrait)
		result += UIInterfaceOrientationMaskPortrait;
	
	if (_params.supportedInterfaceOrientations & InterfaceOrientation_PortraitUpsideDown)
		result += UIInterfaceOrientationMaskPortraitUpsideDown;
	
	if (_params.supportedInterfaceOrientations & InterfaceOrientation_LandscapeLeft)
		result += UIInterfaceOrientationMaskLandscapeLeft;
	
	if (_params.supportedInterfaceOrientations & InterfaceOrientation_LandscapeRight)
		result += UIInterfaceOrientationMaskLandscapeRight;
	
	return result;
}

- (BOOL)shouldAutorotate
{
	return _params.supportedInterfaceOrientations != 0;
}

- (BOOL)prefersStatusBarHidden
{
	return YES;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
{
	if ((toInterfaceOrientation == UIInterfaceOrientationLandscapeLeft) &&
		(_params.supportedInterfaceOrientations & InterfaceOrientation_LandscapeLeft)) return YES;
	
	if ((toInterfaceOrientation == UIInterfaceOrientationLandscapeRight) &&
		(_params.supportedInterfaceOrientations & InterfaceOrientation_LandscapeRight)) return YES;
	
	if ((toInterfaceOrientation == UIInterfaceOrientationPortrait) &&
		(_params.supportedInterfaceOrientations & InterfaceOrientation_Portrait)) return YES;
	
	if ((toInterfaceOrientation == UIInterfaceOrientationPortraitUpsideDown) &&
		(_params.supportedInterfaceOrientations & InterfaceOrientation_PortraitUpsideDown)) return YES;
	
    return NO;
}

- (void)beginRender
{
	[_glView beginRender];
}

- (void)endRender
{
	[_glView endRender];
}

- (void)presentViewController:(UIViewController*)viewControllerToPresent animated:(BOOL)flag completion:(void(^)())completion
{
	[super presentViewController:viewControllerToPresent animated:flag completion:^()
	{
		if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
		{
			_notifier.notifyDeactivated();
		}
		
		if (completion)
			completion();
	}];
}

- (void)dismissViewControllerAnimated:(BOOL)flag completion:(void (^)())completion
{
	[super dismissViewControllerAnimated:flag completion:^()
	{
		if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
		{
			_notifier.notifyActivated();
		}
		
		if (completion)
			completion();
	}];
}

/*
 * Modal - depreacted
 */
- (void)presentModalViewController:(UIViewController*)modalViewController animated:(BOOL)animated
{
	_notifier.notifyDeactivated();
	[super presentModalViewController:modalViewController animated:animated];
}

- (void)dismissModalViewControllerAnimated:(BOOL)animated
{
	_notifier.notifyActivated();
	[super dismissModalViewControllerAnimated:animated];
}

- (BOOL)canBecomeFirstResponder
{
	return YES;
}

- (void)motionBegan:(UIEventSubtype)motion withEvent:(UIEvent*)event
{
	if (motion == UIEventSubtypeMotionShake)
	{
		Input::GestureInputSource gestureInput;
		gestureInput.gesturePerformed(GestureInputInfo(GestureTypeMask_Shake));
	}
	(void)event;
}

- (void)onNotificationRecevied:(NSNotification*)notification
{
	if ([notification.name isEqualToString:etKeyboardRequiredNotification])
		[self performSelectorOnMainThread:@selector(resignFirstResponder) withObject:nil waitUntilDone:NO];
	else if ([notification.name isEqualToString:etKeyboardNotRequiredNotification])
		[self performSelectorOnMainThread:@selector(becomeFirstResponder) withObject:nil waitUntilDone:NO];
}

@end

#endif // ET_PLATFORM_IOS
