/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <et/app/applicationnotifier.h>

#if defined(ET_HAVE_COCOS)

#include <cocos2d.h>
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

- (oneway void)release { }
- (NSUInteger)retainCount { return LONG_MAX; }
- (id)retain { return self; }
- (id)autorelease { return self; }

- (id)init
{
	self = [super init];
	if (self)
	{
		_loaded = NO;
	}
	return self;
}

- (void)loadedInViewController:(UIViewController*)viewController withView:(UIView*)view
{
	NSAssert(_loaded == NO, @"Method [etApplication loaded] should be called once.");
	
	[view addObserver:self forKeyPath:@"frame" options:NSKeyValueObservingOptionNew context:nil];

	RenderState::State state = RenderState::currentState();
	int defaultFramebuffer = [[view valueForKey:@"renderer_"] defaultFrameBuffer];

	application().run(0, 0);
	
	_notifier.accessRenderContext()->renderState().setDefaultFramebuffer(
		[self renderContext]->framebufferFactory().createFramebufferWrapper(defaultFramebuffer));

	_notifier.accessRenderContext()->renderState().applyState(state);
	_loaded = YES;
}

- (void)loadedInViewController:(UIViewController*)vc
{
	CCGLView* view = (CCGLView*)[[CCDirector sharedDirector] view];
	NSAssert(view, @"Cocos OpenGL view should be initialized before running embedded application.");

	[self loadedInViewController:vc withView:view];
}

- (void)unloadedInViewController:(UIViewController*)viewController
{
    _loaded = NO;
	application().quit();
}

- (void)dealloc
{
    _loaded = NO;
	application().quit();
	[super dealloc];
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
		
		_notifier.accessRenderContext()->renderState().defaultFramebuffer()->forceSize(size);
		_notifier.notifyResize(size);
	}
}

- (et::RenderContext*)renderContext
{
	return _notifier.accessRenderContext();
}

- (et::IApplicationDelegate*)applicationDelegate
{
	return application().delegate();
}

@end

#else
#	warning define ET_HAVE_COCOS to compile Cocos platform
#endif // ET_HAVE_COCOS
