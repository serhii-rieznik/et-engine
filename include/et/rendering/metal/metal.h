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
#include <et/rendering/base/rendering.h>
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

struct MetalNativeBuffer
{
	id<MTLBuffer> buffer = nil;
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
