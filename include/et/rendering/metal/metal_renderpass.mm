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
	MTLRenderPassDescriptor* descriptor = nil;

	MetalRenderPassPrivate(MetalState& s)
		: state(s) { }
};

MetalRenderPass::MetalRenderPass(MetalRenderer* renderer, MetalState& state,
    const RenderPass::ConstructionInfo& info) : RenderPass(info)
{
	ET_PIMPL_INIT(MetalRenderPass, state)
    _private->renderer = renderer;

	_private->descriptor = ET_OBJC_RETAIN([MTLRenderPassDescriptor renderPassDescriptor]);
	_private->descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
	_private->descriptor.colorAttachments[0].storeAction = MTLStoreActionDontCare;
	_private->descriptor.colorAttachments[0].clearColor = MTLClearColorMake(info.target.clearColor.x, info.target.clearColor.y, info.target.clearColor.z, info.target.clearColor.w);
	_private->descriptor.depthAttachment.loadAction = MTLLoadActionClear;
	_private->descriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
	_private->descriptor.depthAttachment.clearDepth = info.target.clearDepth;
}

MetalRenderPass::~MetalRenderPass()
{
	ET_OBJC_RELEASE(_private->descriptor);
	ET_PIMPL_FINALIZE(MetalRenderPass)
}

void MetalRenderPass::begin()
{
	ET_ASSERT(_private->state.mainDrawable != nil);

	_private->descriptor.colorAttachments[0].texture = _private->state.mainDrawable.texture;
	_private->descriptor.depthAttachment.texture = _private->state.defaultDepthBuffer;
	_private->encoder.encoder = [_private->state.mainCommandBuffer renderCommandEncoderWithDescriptor:_private->descriptor];
}

void MetalRenderPass::pushRenderBatch(RenderBatch::Pointer batch)
{
	MaterialInstance::Pointer material = batch->material();

	MetalPipelineState::Pointer ps = _private->renderer->createPipelineState(RenderPass::Pointer(this), material->base(), batch->vertexStream());
	ps->setObjectVariable(PipelineState::kWorldTransform(), batch->transformation());
	ps->setObjectVariable(PipelineState::kWorldRotationTransform(), batch->rotationTransformation());
	ps->bind(_private->encoder, material);

	SharedVariables& sharedVariables = _private->renderer->sharedVariables();
	if (info().camera.valid())
	{
		sharedVariables.loadCameraProperties(info().camera);
	}
	if (info().light.valid())
	{
		sharedVariables.loadLightProperties(info().light);
	}
	MetalDataBuffer::Pointer sv = sharedVariables.buffer();
	[_private->encoder.encoder setVertexBuffer:sv->nativeBuffer().buffer() offset:0 atIndex:PassVariablesBufferIndex];

	MetalVertexBuffer::Pointer vb = batch->vertexStream()->vertexBuffer();
	[_private->encoder.encoder setVertexBuffer:vb->nativeBuffer().buffer() offset:0 atIndex:VertexStreamBufferIndex];

	MetalIndexBuffer::Pointer ib = batch->vertexStream()->indexBuffer();
	[_private->encoder.encoder drawIndexedPrimitives:metal::primitiveTypeValue(ib->primitiveType())
		indexCount:batch->numIndexes() indexType:metal::indexArrayFormat(ib->format())
		indexBuffer:ib->nativeBuffer().buffer() indexBufferOffset:ib->byteOffsetForIndex(batch->firstIndex())];
}

void MetalRenderPass::end()
{
	_private->renderer->sharedVariables().flushBuffer();
	_private->renderer->sharedConstBuffer().flush();

	[_private->encoder.encoder endEncoding];
	_private->encoder.encoder = nil;
}

}
