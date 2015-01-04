/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/applicationnotifier.h>

#if (ET_PLATFORM_IOS)

#include <GLKit/GLKit.h>
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
	ET_ASSERT(_loaded == NO);
	ET_ASSERT([EAGLContext currentContext] != nil)
	(void)viewController;
	
	RenderState::State state = RenderState::currentState();
	GLint defaultFrameBufferId = state.boundFramebuffer;

	application().run(0, nullptr);

	Framebuffer::Pointer defaultFrameBuffer =
		[self renderContext]->framebufferFactory().createFramebufferWrapper(defaultFrameBufferId);
	
	vec2i contextSize(static_cast<int>(view.bounds.size.width), static_cast<int>(view.bounds.size.height));
	defaultFrameBuffer->forceSize(contextSize);

	_notifier.accessRenderContext()->renderState().setDefaultFramebuffer(defaultFrameBuffer);
	_notifier.notifyResize(contextSize);
	_notifier.accessRenderContext()->renderState().applyState(state);
	
	_loaded = YES;
}

- (void)unloadedInViewController:(UIViewController*)viewController
{
	ET_ASSERT(_loaded)
	(void)viewController;
	
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

@end

#endif // ET_PLATFORM_IOS
