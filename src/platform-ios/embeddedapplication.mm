/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <GLKit/GLKit.h>
#include <et/app/applicationnotifier.h>
#include <et/platform-ios/embeddedapplication.h>

using namespace et;

@interface etApplication()
{
	ApplicationNotifier _notifier;
	BOOL _loaded;
}

@end

@implementation etApplication

static etApplication* _sharedInstance = nil;

/*
 * Singletone Methods
 */
+ (etApplication*)sharedApplication
{
	if (_sharedInstance == nil)
		_sharedInstance = [[etApplication alloc] init];
	
	return _sharedInstance;
}

#if (!ET_OBJC_ARC_ENABLED)
- (oneway void)release { }
- (NSUInteger)retainCount { return LONG_MAX; }
- (id)retain { return self; }
- (id)autorelease { return self; }
#endif

- (id)init
{
	self = [super init];
	if (self)
	{
		_loaded = NO;
	}
	return self;
}

- (void)loadedInViewController:(UIViewController*)viewController
{
	[self loadedInViewController:viewController withView:viewController.view];
}

- (void)loadedInViewController:(UIViewController*)viewController withView:(UIView*)view
{
	NSAssert(_loaded == NO, @"Method [etApplication loadedInViewController:] should be called once.");
	
	RenderState::State state = RenderState::currentState();
	GLint defaultFrameBufferId = state.boundFramebuffer;

	application().run(0, nullptr);

	Framebuffer::Pointer defaultFrameBuffer =
		[self renderContext]->framebufferFactory().createFramebufferWrapper(defaultFrameBufferId);
	
	vec2i contextSize(view.bounds.size.width, view.bounds.size.height);
	defaultFrameBuffer->forceSize(contextSize);

	_notifier.accessRenderContext()->renderState().setDefaultFramebuffer(defaultFrameBuffer);
	_notifier.notifyResize(contextSize);
	_notifier.accessRenderContext()->renderState().applyState(state);

	[view addObserver:self forKeyPath:@"frame" options:NSKeyValueObservingOptionNew context:nil];
	
	_loaded = YES;
}

- (void)unloadedInViewController:(UIViewController*)viewController
{
	NSAssert(_loaded == YES, @"Method [etApplication unloadedInViewController:] should be called once.");
	
	application().quit();
	_loaded = NO;
}

- (void)dealloc
{
	application().quit();
	
#if (!ET_OBJC_ARC_ENABLED)
	[super dealloc];
#endif
}

- (et::RenderContext*)renderContext
{
	return _notifier.accessRenderContext();
}

- (et::IApplicationDelegate*)applicationDelegate
{
	return application().delegate();
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object
	change:(NSDictionary *)change context:(void *)context
{
	if ([keyPath isEqualToString:@"frame"])
	{
		float scaleFactor = [[UIScreen mainScreen] scale];
		
		CGRect frame = { };
		NSValue* value = [change objectForKey:@"new"];
		[value getValue:&frame];
		
		vec2i size(static_cast<int>(scaleFactor * frame.size.width),
			static_cast<int>(scaleFactor * frame.size.height));
		
		RenderContext* rc = _notifier.accessRenderContext();
		if (rc->sizei() != size)
		{
			rc->renderState().defaultFramebuffer()->forceSize(size);
			_notifier.notifyResize(size);
		}
	}
}

@end
