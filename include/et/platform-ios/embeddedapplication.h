/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#import <UIKit/UIKit.h>

namespace et
{
	class RenderContext;
	class IApplicationDelegate;
}

@interface etApplication : NSObject

+ (etApplication*)sharedApplication;

- (void)loadedInViewController:(UIViewController*)viewController;
- (void)loadedInViewController:(UIViewController*)viewController withView:(UIView*)view;

- (void)unloadedInViewController:(UIViewController*)viewController;

- (et::RenderContext*)renderContext;
- (et::IApplicationDelegate*)applicationDelegate;

@end
