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
	MetalState& state;

	MetalRenderPassPrivate(MetalState& s)
		: state(s) { }
};

MetalRenderPass::MetalRenderPass(MetalRenderer* renderer, MetalState& state,
    const RenderPass::ConstructionInfo& info) : RenderPass(info)
{
	ET_PIMPL_INIT(MetalRenderPass, state)
    _private->renderer = renderer;

	ET_ASSERT(state.mainDrawable != nil);

	MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];

	pass.colorAttachments[0].texture = state.mainDrawable.texture;
	pass.colorAttachments[0].loadAction = MTLLoadActionClear;
	pass.colorAttachments[0].storeAction = MTLStoreActionDontCare;
	pass.colorAttachments[0].clearColor = MTLClearColorMake(info.target.clearColor.x, info.target.clearColor.y, info.target.clearColor.z, info.target.clearColor.w);

	pass.depthAttachment.texture = state.defaultDepthBuffer;
	pass.depthAttachment.loadAction = MTLLoadActionClear;
	pass.depthAttachment.storeAction = MTLStoreActionDontCare;
	pass.depthAttachment.clearDepth = info.target.clearDepth;
	
	_private->encoder = [state.mainCommandBuffer renderCommandEncoderWithDescriptor:pass];
}

MetalRenderPass::~MetalRenderPass()
{
	ET_PIMPL_FINALIZE(MetalRenderPass)
}

void MetalRenderPass::pushRenderBatch(RenderBatch::Pointer batch)
{
	const Camera& cam = info().camera;

    MetalPipelineState::Pointer ps = _private->renderer->createPipelineState(RenderPass::Pointer(this), batch->material(), batch->vao());
	ps->build();
	
    MetalVertexBuffer::Pointer vb = batch->vao()->vertexBuffer();
    MetalIndexBuffer::Pointer ib = batch->vao()->indexBuffer();

	uint8_t* uniformData = reinterpret_cast<uint8_t*>([ps->uniformsBuffer().buffer() contents]);
	memcpy(uniformData, cam.viewProjectionMatrix().data(), sizeof(mat4));
	memcpy(uniformData + sizeof(mat4), batch->transformation().data(), sizeof(mat4));

    [_private->encoder setRenderPipelineState:ps->nativeState().pipelineState];
    [_private->encoder setDepthStencilState:ps->nativeState().depthStencilState];
    [_private->encoder setVertexBuffer:vb->nativeBuffer().buffer() offset:0 atIndex:0];

	for (MTLArgument* arg in ps->nativeState().reflection.vertexArguments)
	{
		if (arg.type == MTLArgumentTypeTexture)
		{
			MetalTexture::Pointer tex = batch->material()->texture([arg.name UTF8String]);
			ET_ASSERT(tex.valid());
			[_private->encoder setVertexTexture:tex->nativeTexture().texture atIndex:arg.index];
		}
		else if (arg.type == MTLArgumentTypeBuffer)
		{
			if ([arg.name isEqualToString:@"uniforms"])
			{
				[_private->encoder setVertexBuffer:ps->uniformsBuffer().buffer() offset:0 atIndex:arg.index];
			}
		}
	}

	for (MTLArgument* arg in ps->nativeState().reflection.fragmentArguments)
	{
		if (arg.type == MTLArgumentTypeTexture)
		{
			MetalTexture::Pointer tex = batch->material()->texture([arg.name UTF8String]);
			ET_ASSERT(tex.valid());
			[_private->encoder setFragmentTexture:tex->nativeTexture().texture atIndex:arg.index];
		}
		else if (arg.type == MTLArgumentTypeBuffer)
		{
			if ([arg.name isEqualToString:@"uniforms"])
			{
				[_private->encoder setFragmentBuffer:ps->uniformsBuffer().buffer() offset:0 atIndex:arg.index];
			}
		}
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
