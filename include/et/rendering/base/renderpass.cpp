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
	_sharedTextures[texId].first = tex;
	_sharedTextures[texId].second = smp;
	_sharedTexturesSetValid = false;
}

const TextureSet::Pointer& RenderPass::sharedTexturesSet()
{
	if (!_sharedTexturesSetValid)
		buildSharedTexturesSet();

	return _sharedTexturesSet;
}

void RenderPass::buildSharedTexturesSet()
{
	TextureSet::Description desc;
	for (const auto& i : _sharedTextures)
	{
		desc.vertexTextures.emplace(static_cast<uint32_t>(i.first), i.second.first);
		desc.vertexSamplers.emplace(static_cast<uint32_t>(i.first), i.second.second);
		desc.fragmentTextures.emplace(static_cast<uint32_t>(i.first), i.second.first);
		desc.fragmentSamplers.emplace(static_cast<uint32_t>(i.first), i.second.second);
	}
	_sharedTexturesSet = _renderer->createTextureSet(desc);
	_sharedTexturesSetValid = true;
}

}
