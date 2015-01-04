/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */
#pragma once

#import <UIKit/UIKit.h>
#import <et/rendering/framebuffer.h>
#import <et/input/input.h>
#import <et/rendering/rendercontextparams.h>

@interface etOpenGLView : UIView <UIKeyInput>

- (id)initWithFrame:(CGRect)frame parameters:(et::RenderContextParameters&)params;

- (void)setRenderContext:(et::RenderContext*)rc;

- (void)beginRender;
- (void)endRender;

- (const et::Framebuffer::Pointer&)defaultFramebuffer;

@property (nonatomic, retain) EAGLContext* context;

@end
