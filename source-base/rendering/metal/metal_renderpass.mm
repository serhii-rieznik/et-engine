/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal.h>
#include <et/rendering/metal/metal_renderpass.h>
#include <et/rendering/metal/metal_pipelinestate.h>
#include <et/rendering/metal/metal_vertexbuffer.h>
#include <et/rendering/metal/metal_indexbuffer.h>

namespace et
{

class MetalRenderPassPrivate
{
public:
	id<MTLRenderCommandEncoder> encoder = nil;
};

MetalRenderPass::MetalRenderPass(MetalState& state, const RenderPass::ConstructionInfo& info) :
	RenderPass(info)
{
	ET_PIMPL_INIT(MetalRenderPass)

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
	MetalPipelineState::Pointer ps = batch->material()->createPipelineState();
	ps->setVertexStream(batch->vao());
	ps->build();
    
    MetalVertexBuffer::Pointer vb = batch->vao()->vertexBuffer();
    MetalIndexBuffer::Pointer ib = batch->vao()->indexBuffer();
    
    [_private->encoder setRenderPipelineState:ps->nativeState().pipelineState];
    [_private->encoder setDepthStencilState:ps->nativeState().depthStencilState];
    [_private->encoder setVertexBuffer:vb->nativeBuffer().buffer() offset:0 atIndex:0];
    
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
