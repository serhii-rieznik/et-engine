/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#if defined(__OBJC__)

#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>
#include <QuartzCore/CAMetalLayer.h>
#include <et/rendering/rendering.h>
#include <et/platform-apple/objc.h>

namespace et
{

class MetalState
{
public:
	CAMetalLayer* layer = nil;
	
	id<MTLDevice> device = nil;
	id<MTLCommandQueue> queue = nil;

	id<MTLCommandBuffer> mainCommandBuffer = nil;
	id<CAMetalDrawable> mainDrawable = nil;
};

namespace metal
{
    MTLTextureType textureTargetValue(TextureTarget, uint32_t samples);
    MTLPixelFormat textureFormatValue(TextureFormat);
}

}
#endif
