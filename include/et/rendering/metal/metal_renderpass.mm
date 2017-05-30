/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal.h>
#include <et/rendering/metal/metal_renderer.h>
#include <et/rendering/metal/metal_renderpass.h>
#include <et/rendering/metal/metal_buffer.h>
#include <et/rendering/metal/metal_texture.h>
#include <et/rendering/metal/metal_pipelinestate.h>

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
    const RenderPass::ConstructionInfo& info) : RenderPass(renderer, info)
{
	ET_PIMPL_INIT(MetalRenderPass, state);
    _private->renderer = renderer;

	const vec4& cl0 = info.color[0].clearValue;

	_private->descriptor = ET_OBJC_RETAIN([MTLRenderPassDescriptor renderPassDescriptor]);
	_private->descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
	_private->descriptor.colorAttachments[0].storeAction = MTLStoreActionDontCare;
	_private->descriptor.colorAttachments[0].clearColor = MTLClearColorMake(cl0.x, cl0.y, cl0.z, cl0.w);
	_private->descriptor.depthAttachment.loadAction = MTLLoadActionClear;
	_private->descriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
	_private->descriptor.depthAttachment.clearDepth = info.depth.clearValue.x;
}

MetalRenderPass::~MetalRenderPass()
{
	ET_OBJC_RELEASE(_private->descriptor);
	ET_PIMPL_FINALIZE(MetalRenderPass);
}

void MetalRenderPass::begin(const RenderPassBeginInfo& /* info */)
{
	ET_ASSERT(_private->state.mainDrawable != nil);

	_private->descriptor.colorAttachments[0].texture = _private->state.mainDrawable.texture;
	_private->descriptor.depthAttachment.texture = _private->state.defaultDepthBuffer;
	_private->encoder.encoder = [_private->state.mainCommandBuffer renderCommandEncoderWithDescriptor:_private->descriptor];
}

void MetalRenderPass::pushRenderBatch(const RenderBatch::Pointer&)
{
	/*
	 * TODO : rewrite / update
	 *
	MaterialInstance::Pointer material = batch->material();

	MetalPipelineState::Pointer ps = _private->renderer->acquirePipelineState(RenderPass::Pointer(this), material->base(), batch->vertexStream());
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
	*/
}

void MetalRenderPass::end()
{
	// TODO : move to the correct place
	// _private->renderer->sharedConstantBuffer().flush();
	[_private->encoder.encoder endEncoding];
	_private->encoder.encoder = nil;
}

void MetalRenderPass::pushImageBarrier(const Texture::Pointer&, const ResourceBarrier&)
{
}

void MetalRenderPass::copyImage(const Texture::Pointer&, const Texture::Pointer&, const CopyDescriptor&)
{
}

void MetalRenderPass::dispatchCompute(const Compute::Pointer&, const vec3i&)
{
}

void MetalRenderPass::nextSubpass()
{
}

}
