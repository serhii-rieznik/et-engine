/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/interface/renderpass.h>
#include <et/rendering/interface/renderer.h>

namespace et
{
const std::string RenderPass::kPassNameForward = "forward";
const std::string RenderPass::kPassNameDepth = "depth";
const std::string RenderPass::kPassNameUI = "ui";

RenderPass::RenderPass(RenderInterface* renderer, const ConstructionInfo& info) :
	_renderer(renderer), _info(info)
{
	_dynamicConstantBuffer.init(renderer);
}

RenderPass::~RenderPass()
{
	_dynamicConstantBuffer.shutdown();
}

const RenderPass::ConstructionInfo& RenderPass::info() const
{
	return _info;
}

ConstantBuffer& RenderPass::dynamicConstantBuffer()
{
	return _dynamicConstantBuffer;
}

void RenderPass::setSharedTexture(MaterialTexture texId, const Texture::Pointer& tex, const Sampler::Pointer& smp)
{
	ET_ASSERT(texId >= MaterialTexture::FirstSharedTexture);
	ET_ASSERT(texId <= MaterialTexture::LastSharedTexture);
	_sharedTextures[texId].first = tex;
	_sharedTextures[texId].second = smp;
}

uint64_t RenderPass::identifier() const
{
	return reinterpret_cast<uintptr_t>(this);
}

void RenderPass::executeSingleRenderBatch(const RenderBatch::Pointer& batch, const RenderPassBeginInfo& info)
{
	begin(info);
	pushRenderBatch(batch);
	end();
}

void RenderPass::loadSharedVariablesFromCamera(const Camera::Pointer& cam)
{
	setSharedVariable(ObjectVariable::ViewTransform, cam->viewMatrix());
	setSharedVariable(ObjectVariable::InverseViewTransform, cam->inverseViewMatrix());

	setSharedVariable(ObjectVariable::ProjectionTransform, cam->projectionMatrix());
	setSharedVariable(ObjectVariable::InverseProjectionTransform, cam->inverseProjectionMatrix());

	setSharedVariable(ObjectVariable::ViewProjectionTransform, cam->viewProjectionMatrix());
	setSharedVariable(ObjectVariable::InverseViewProjectionTransform, cam->inverseViewProjectionMatrix());
	
	setSharedVariable(ObjectVariable::CameraPosition, cam->position());
	setSharedVariable(ObjectVariable::CameraDirection, cam->direction());
}

void RenderPass::loadSharedVariablesFromLight(const Light::Pointer& l)
{
	setSharedVariable(ObjectVariable::LightDirection, -l->direction());
	setSharedVariable(ObjectVariable::LightViewTransform, l->viewMatrix());
	setSharedVariable(ObjectVariable::LightProjectionTransform, l->projectionMatrix());
}

}
