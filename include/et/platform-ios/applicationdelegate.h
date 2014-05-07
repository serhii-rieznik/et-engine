/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#import <UIKit/UIApplication.h>
#import <et/app/events.h>

namespace et
{
	class ApplicationNotifier;
}

@interface etApplicationDelegate : UIResponder <UIApplicationDelegate> 

@property (nonatomic, retain) UIWindow* window;

- (void)beginUpdates;
- (void)endUpdates;

@end