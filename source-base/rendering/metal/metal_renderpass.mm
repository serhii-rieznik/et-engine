/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal.h>
#include <et/rendering/metal/metal_renderer.h>
#include <et/rendering/metal/metal_renderpass.h>
#include <et/rendering/metal/metal_pipelinestate.h>
#include <et/rendering/metal/metal_vertexbuffer.h>
#include <et/rendering/metal/metal_indexbuffer.h>
#include <et/rendering/metal/metal_texture.h>

namespace et
{

class MetalRenderPassPrivate
{
public:
	id<MTLRenderCommandEncoder> encoder = nil;
    MetalRenderer* renderer = nullptr;
};

MetalRenderPass::MetalRenderPass(MetalRenderer* renderer, MetalState& state,
    const RenderPass::ConstructionInfo& info) : RenderPass(info)
{
	ET_PIMPL_INIT(MetalRenderPass)
    _private->renderer = renderer;

	ET_ASSERT(state.mainDrawable != nil);

	MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
	pass.colorAttachments[0].texture = state.mainDrawable.texture;
	pass.colorAttachments[0].loadAction = MTLLoadActionClear;
	pass.colorAttachments[0].storeAction = MTLStoreActionDontCare;
	pass.colorAttachments[0].clearColor = MTLClearColorMake(info.target.clearColor.x, info.target.clearColor.y, info.target.clearColor.z, info.target.clearColor.w);
	
	_private->encoder = [state.mainCommandBuffer renderCommandEncoderWithDescriptor:pass];
}

MetalRenderPass::~MetalRenderPass()
{
	ET_PIMPL_FINALIZE(MetalRenderPass)
}

void MetalRenderPass::pushRenderBatch(RenderBatch::Pointer batch)
{
    MetalPipelineState::Pointer ps = _private->renderer->createPipelineState(RenderPass::Pointer(this), batch->material(), batch->vao());

    MetalVertexBuffer::Pointer vb = batch->vao()->vertexBuffer();
    MetalIndexBuffer::Pointer ib = batch->vao()->indexBuffer();

    ps->build();

    [_private->encoder setRenderPipelineState:ps->nativeState().pipelineState];
    [_private->encoder setDepthStencilState:ps->nativeState().depthStencilState];
    [_private->encoder setVertexBuffer:vb->nativeBuffer().buffer() offset:0 atIndex:0];
	// MTLViewport vp = { 0.0f, 0.0f, 640.0f, 480.0f, 1.0f, 1.0f };
	// [_private->encoder setViewport:vp];

	MetalTexture::Pointer tex = batch->material()->texture("color_texture");
	if (tex.valid())
	{
		[_private->encoder setFragmentTexture:tex->nativeTexture().texture atIndex:0];
	}

	[_private->encoder drawIndexedPrimitives:metal::primitiveTypeValue(ib->primitiveType())
                                  indexCount:batch->numIndexes()
                                   indexType:MTLIndexTypeUInt16
                                 indexBuffer:ib->nativeBuffer().buffer()
                           indexBufferOffset:batch->firstIndex()];
}

void MetalRenderPass::endEncoding()
{
	[_private->encoder endEncoding];
	_private->encoder = nil;
}

}
