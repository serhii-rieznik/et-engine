/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <ccGLState.h>
#include <et/platform-cocos/etnode.h>
#include <et/app/applicationnotifier.h>

using namespace et;

@interface etNode()
{
	ApplicationNotifier* _notifier;
}

@end

@implementation etNode

- (id)init
{
	self = [super init];
	if (self)
	{
		_notifier = new ApplicationNotifier();
	}
	return self;
}

- (void)dealloc
{
	delete _notifier;
	[super dealloc];
}

- (void)onEnter
{
	[super onEnter];
	_notifier->notifyActivated();
}

- (void)onExit
{
	[super onExit];
	_notifier->notifyDeactivated();
}

- (void)draw
{
	PreservedRenderStateScope lock(_notifier->accessRenderContext()->renderState(), true);
	_notifier->notifyIdle();
}

@end
