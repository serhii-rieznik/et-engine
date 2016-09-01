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

struct MetalState
{
	CAMetalLayer* layer = nil;
	
	id<MTLDevice> device = nil;
	id<MTLCommandQueue> queue = nil;

	id<MTLCommandBuffer> mainCommandBuffer = nil;
	id<CAMetalDrawable> mainDrawable = nil;

	id<MTLTexture> defaultDepthBuffer = nil;
};
    
struct MetalNativeProgram
{
    id<MTLFunction> vertexFunction = nil;
    id<MTLFunction> fragmentFunction = nil;
};
    
struct MetalNativeTexture
{
    id<MTLTexture> texture = nil;
};

struct MetalNativePipelineState
{
    id<MTLRenderPipelineState> pipelineState = nil;
    id<MTLDepthStencilState> depthStencilState = nil;
    
    MTLRenderPipelineReflection* reflection = nil;
};

class MetalNativeBuffer
{
public:
    MetalNativeBuffer() = default;
    
    MetalNativeBuffer(MetalState& metal, const void* data, uint32_t size)
	{
		_buffer = [metal.device newBufferWithBytes:data length:size options:MTLResourceCPUCacheModeDefaultCache];
	}

	MetalNativeBuffer(MetalState& metal, uint32_t size)
	{
		_buffer = [metal.device newBufferWithLength:size options:MTLResourceCPUCacheModeDefaultCache];
	}

    ~MetalNativeBuffer()
        { ET_OBJC_RELEASE(_buffer); }
    
    id<MTLBuffer> buffer() const
        { return _buffer; }
    
private:
    id<MTLBuffer> _buffer = nil;
};
    
namespace metal
{
    MTLTextureType textureTargetValue(TextureTarget, uint32_t samples);
    MTLPixelFormat textureFormatValue(TextureFormat);
    MTLPrimitiveType primitiveTypeValue(PrimitiveType);
    MTLPrimitiveTopologyClass primitiveTypeToTopology(PrimitiveType);
    MTLVertexFormat dataTypeToVertexFormat(DataType);
	MTLCompareFunction compareFunctionValue(CompareFunction);
}

}
#else
#	error This file should only be included in .mm
#endif
