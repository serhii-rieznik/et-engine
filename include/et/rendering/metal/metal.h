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

struct MetalNativeEncoder
{
	id<MTLRenderCommandEncoder> encoder = nil;
};

struct MetalNativeSampler
{
	id<MTLSamplerState> sampler = nil;
};

class MetalNativeBuffer : public Shared
{
public:
	ET_DECLARE_POINTER(MetalNativeBuffer);

public:
	MetalNativeBuffer() = default;
	MetalNativeBuffer(const MetalNativeBuffer&) = delete;
	MetalNativeBuffer(MetalNativeBuffer&&) = delete;
	MetalNativeBuffer& operator = (const MetalNativeBuffer&) = delete;

    MetalNativeBuffer(MetalState& metal, const void* data, uint32_t size)
	{
		construct(metal, data, size);
	}

	MetalNativeBuffer(MetalState& metal, uint32_t size)
	{
		construct(metal, size);
	}

	void construct(MetalState& metal, const void* data, uint32_t size)
	{
		_buffer = [metal.device newBufferWithBytes:data length:size options:MTLResourceCPUCacheModeDefaultCache];
	}

	void construct(MetalState& metal, uint32_t size)
	{
		_buffer = [metal.device newBufferWithLength:size options:MTLResourceStorageModeShared];
	}

    ~MetalNativeBuffer()
        { ET_OBJC_RELEASE(_buffer); }
    
    id<MTLBuffer> buffer() const
        { return _buffer; }

	uint8_t* bufferContents() const
		{ return reinterpret_cast<uint8_t*>([_buffer contents]); }

	bool valid() const
		{ return _buffer != nil; }
    
private:
    id<MTLBuffer> _buffer = nil;
};
    
namespace metal
{
    MTLTextureType textureTargetValue(TextureTarget, uint32_t samples);
    MTLPixelFormat textureFormatValue(TextureFormat);
	MTLPixelFormat renderableTextureFormatValue(TextureFormat);
	MTLPrimitiveType primitiveTypeValue(PrimitiveType);
    MTLPrimitiveTopologyClass primitiveTypeToTopology(PrimitiveType);
    MTLVertexFormat dataTypeToVertexFormat(DataType);
	MTLCompareFunction compareFunctionValue(CompareFunction);
	MTLIndexType indexArrayFormat(IndexArrayFormat);
	MTLSamplerAddressMode wrapModeToAddressMode(TextureWrap);
	MTLSamplerMipFilter textureFilteringToMipFilter(TextureFiltration);
	MTLSamplerMinMagFilter textureFilteringToSamplerFilter(TextureFiltration);

	DataType mtlDataTypeToDataType(MTLDataType);
}

}
#else
#	error This file should only be included in .mm
#endif
