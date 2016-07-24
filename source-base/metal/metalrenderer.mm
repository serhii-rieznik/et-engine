/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/metal/metalrenderer.h>
#include <et/platform-apple/objc.h>

#if (ET_PLATFORM_MAC)
#	include <Metal/Metal.h>
#	include <MetalKit/MetalKit.h>
#else
#	error Not implemented for this platform
#endif

namespace et
{

class MetalRendererPrivate
{
public:
	id<MTLDevice> device = nullptr;
	id<MTLCommandQueue> queue = nullptr;
	id<CAMetalDrawable> currentDrawable = nullptr;
	id<MTLCommandBuffer> currentBuffer = nullptr;
	id<MTLRenderCommandEncoder> encoder = nullptr;
	CAMetalLayer* layer = nullptr;
};

MetalRenderer::MetalRenderer(RenderContext* rc)
	: RenderInterface(rc)
{
	ET_PIMPL_INIT(MetalRenderer);
}

MetalRenderer::~MetalRenderer()
{
	ET_PIMPL_FINALIZE(MetalRenderer)
}

void MetalRenderer::init(const RenderContextParameters& params)
{
	_private->device = MTLCreateSystemDefaultDevice();
	_private->queue = [_private->device newCommandQueue];

	_private->layer = [[CAMetalLayer alloc] init];
	_private->layer.device = _private->device;
	_private->layer.framebufferOnly = YES;
	_private->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

	application().context().objects[3] = (__bridge void*)(_private->device);
	application().context().objects[4] = (__bridge void*)_private->layer;
}

void MetalRenderer::shutdown()
{
	ET_OBJC_RELEASE(_private->queue);
	ET_OBJC_RELEASE(_private->device);
}

void MetalRenderer::begin()
{
	_private->currentDrawable = [_private->layer nextDrawable];
	_private->currentBuffer = [_private->queue commandBuffer];
}

void MetalRenderer::present()
{
	[_private->currentBuffer presentDrawable:_private->currentDrawable];
	_private->currentDrawable = nil;
	
	[_private->currentBuffer commit];
	_private->currentBuffer = nil;
}

void MetalRenderer::drawIndexedPrimitive(PrimitiveType pt, IndexArrayFormat fmt, uint32_t first, uint32_t count)
{
}

RenderPass::Pointer MetalRenderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	RenderPass::Pointer result = RenderPass::Pointer::create(info);

	MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
	pass.colorAttachments[0].texture = _private->currentDrawable.texture;
	pass.colorAttachments[0].loadAction = MTLLoadActionClear;
	pass.colorAttachments[0].storeAction = MTLStoreActionDontCare;
	pass.colorAttachments[0].clearColor = MTLClearColorMake(info.target.clearColor.x, info.target.clearColor.y, info.target.clearColor.z, info.target.clearColor.w);
	_private->encoder = [_private->currentBuffer renderCommandEncoderWithDescriptor:pass];

	return result;
}

void MetalRenderer::submitRenderPass(RenderPass::Pointer pass)
{
	[_private->encoder endEncoding];
}

}
