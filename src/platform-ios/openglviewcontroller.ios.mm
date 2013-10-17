/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#import <QuartzCore/QuartzCore.h>
#import <et/platform-ios/openglviewcontroller.h>
#import <et/app/applicationnotifier.h>

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

- (id)initWithParameters:(RenderContextParameters)params
{
	self = [super init];
	
	if (self)
	{
		_params = params;
		
		BOOL initialized = [self performInitialization];
		
		assert(initialized);
		(void)initialized;
		
		self.view = _glView;
	}
	return self;
}

- (BOOL)performInitialization
{
#if defined(GL_ES_VERSION_3_0)
	_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
	if (_context == nil)
#endif
	_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	
	_glView = [[etOpenGLView alloc] initWithFrame:[[UIScreen mainScreen] bounds] parameters:_params];
	
	if (!_context)
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

#if defined(__IPHONE_6_0)

- (BOOL)shouldAutorotate
{
	return (_params.supportedInterfaceOrientations > 0);
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

#endif

#if defined(__IPHONE_7_0)

- (BOOL)prefersStatusBarHidden
{
	return YES;
}

#endif

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

- (void)presentModalViewController:(UIViewController *)modalViewController animated:(BOOL)animated
{
	_notifier.notifyDeactivated();
	[super presentModalViewController:modalViewController animated:animated];
}

- (void)dismissModalViewControllerAnimated:(BOOL)animated
{
	[super dismissModalViewControllerAnimated:animated];
	_notifier.notifyActivated();
}

- (BOOL)canBecomeFirstResponder
{
	return YES;
}

- (void)motionBegan:(UIEventSubtype)motion withEvent:(UIEvent *)event
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
