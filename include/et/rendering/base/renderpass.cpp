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

void RenderPass::setCamera(const Camera::Pointer& cam)
{
	_info.camera = cam;
}

void RenderPass::setLightCamera(const Camera::Pointer& cam)
{
	_info.light = cam;
}

Camera::Pointer& RenderPass::camera() 
{
	return _info.camera; 
}

const Camera::Pointer& RenderPass::camera() const
{
	return _info.camera; 
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

}
