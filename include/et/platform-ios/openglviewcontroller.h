/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>

#include <et/core/et.h>
#include <et/rendering/rendercontextparams.h>
#include <et/platform-ios/openglview.h>

@interface etOpenGLViewController : UIViewController

- (id)initWithParameters:(et::RenderContextParameters)params;

- (void)setRenderContext:(et::RenderContext*)rc;

- (void)beginRender;
- (void)endRender;

@property (nonatomic, assign) BOOL autorotationEnabled;
@property (atomic, readonly) EAGLContext* context;

@end
