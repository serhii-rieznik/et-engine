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
	MetalNativeEncoder encoder;
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
	
	_private->encoder.encoder = [state.mainCommandBuffer renderCommandEncoderWithDescriptor:pass];
}

MetalRenderPass::~MetalRenderPass()
{
	ET_PIMPL_FINALIZE(MetalRenderPass)
}

void MetalRenderPass::pushRenderBatch(RenderBatch::Pointer batch)
{
	const Camera& cam = info().camera;
	Material::Pointer material = batch->material();

	SharedVariables& sharedVariables = _private->renderer->variables();
	sharedVariables.loadCameraProperties(cam);

	MetalIndexBuffer::Pointer ib = batch->vertexStream()->indexBuffer();
	MetalVertexBuffer::Pointer vb = batch->vertexStream()->vertexBuffer();

    MetalPipelineState::Pointer ps = _private->renderer->createPipelineState(RenderPass::Pointer(this), batch->material(), batch->vertexStream());
	ps->setProgramVariable("lightPosition", info().defaultLightPosition);
	ps->setProgramVariable("worldTransform", batch->transformation());
	ps->bind(_private->encoder, material);

	MetalDataBuffer::Pointer sv = sharedVariables.buffer();

	[_private->encoder.encoder setVertexBuffer:sv->nativeBuffer().buffer() offset:0 atIndex:SharedVariablesBufferIndex];
	[_private->encoder.encoder setVertexBuffer:vb->nativeBuffer().buffer() offset:0 atIndex:VertexStreamBufferIndex];
	[_private->encoder.encoder drawIndexedPrimitives:metal::primitiveTypeValue(ib->primitiveType())
		indexCount:batch->numIndexes() indexType:metal::indexArrayFormat(ib->format())
		indexBuffer:ib->nativeBuffer().buffer() indexBufferOffset:ib->byteOffsetForIndex(batch->firstIndex())];
}

void MetalRenderPass::endEncoding()
{
	_private->renderer->sharedConstBuffer().flush();
	
	[_private->encoder.encoder endEncoding];
	_private->encoder.encoder = nil;
}

}
