/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */
#pragma once

#import <UIKit/UIKit.h>
#import <et/apiobjects/framebuffer.h>
#import <et/input/input.h>
#import <et/rendering/rendercontextparams.h>

@interface etOpenGLView : UIView <UIKeyInput>

- (id)initWithFrame:(CGRect)frame parameters:(et::RenderContextParameters&)params;

- (void)setRenderContext:(et::RenderContext*)rc;
- (void)beginRender;
- (void)endRender;

- (const et::Framebuffer::Pointer&)defaultFramebuffer;

@property (nonatomic, retain) EAGLContext* context;
@property (atomic, assign) BOOL suspended;

@end
