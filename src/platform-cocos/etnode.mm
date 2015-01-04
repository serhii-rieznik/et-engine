/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/applicationnotifier.h>

#if defined(ET_HAVE_COCOS)

#include <et/platform-cocos/etnode.h>
#include <ccGLState.h>

using namespace et;

@interface etNode()
{
	ApplicationNotifier _notifier;
}

@end

@implementation etNode

- (void)onEnter
{
	[super onEnter];
	_notifier.notifyActivated();
}

- (void)onExit
{
	[super onExit];
	_notifier.notifyDeactivated();
}

- (void)draw
{
	PreservedRenderStateScope lock(_notifier.accessRenderContext(), true);
	
	if (_notifier.shouldPerformRendering())
		_notifier.notifyIdle();
}

@end

#else
#	warning define ET_HAVE_COCOS to compile Cocos platform
#endif // ET_HAVE_COCOS
