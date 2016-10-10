/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>

#include <et/rendering/rendercontextparams.h>
#include <et/platform-ios/openglview.h>

@interface etOpenGLViewController : UIViewController

- (id)initWithParameters:(et::RenderContextParameters)params;

- (void)setRenderContext:(et::RenderContext*)rc;

- (void)beginRender;
- (void)endRender;

@property (atomic, readonly) EAGLContext* context;
@property (atomic, assign) BOOL suspended;

@end
