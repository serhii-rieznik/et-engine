/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <et/rendering/rendercontextparams.h>
#import <et/platform-ios/openglview.h>

@interface etOpenGLViewController : UIViewController

- (id)initWithParameters:(et::RenderContextParameters)params;

- (void)setRenderContext:(et::RenderContext*)rc;

- (void)beginRender;
- (void)endRender;

@property (atomic, readonly) EAGLContext* context;

@end
