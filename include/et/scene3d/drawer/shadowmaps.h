/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/drawer/common.h>

namespace et
{
namespace s3d
{

class ShadowmapProcessor : public Shared
{
public:
	ET_DECLARE_POINTER(ShadowmapProcessor);

public:
	const Texture::Pointer& directionalShadowmap() const;
	const Sampler::Pointer& directionalShadowmapSampler() const;

	void setScene(const Scene::Pointer& scene, Light::Pointer& light);
	void process(RenderInterface::Pointer& renderer, DrawerOptions& options);
	void updateLight(Light::Pointer& light);

	const BoundingBox& sceneBoundingBox() const 
		{ return _sceneBoundingBox; }

private:
	void validate(RenderInterface::Pointer& renderer);
	void setupProjection(DrawerOptions& options);
	void updateConfig(RenderInterface::Pointer& renderer);

private:
	Texture::Pointer _directionalShadowmap;
	Texture::Pointer _directionalShadowmapMoments;
	Texture::Pointer _directionalShadowmapMomentsBuffer;
	Sampler::Pointer _directionalShadowmapSampler;
	Sampler::Pointer _directionalShadowmapMomentsSampler;
	Scene::Pointer _scene;
	BoundingBox _sceneBoundingBox;
	Light::Pointer _light;
	bool _momentsBasedShadowmap = false;

	struct Renderables
	{
		RenderPass::Pointer depthBasedShadowPass;
		RenderPass::Pointer momentsBasedShadowPass;
		Vector<Mesh::Pointer> meshes;

		RenderBatch::Pointer debugColorBatch;
		RenderBatch::Pointer debugDepthBatch;
		RenderPass::Pointer debugPass;

		RenderPass::Pointer blurPass0;
		RenderBatch::Pointer blurBatch0;
		
		RenderPass::Pointer blurPass1;
		RenderBatch::Pointer blurBatch1;

		bool initialized = false;
	} _renderables;
};

inline const Texture::Pointer& ShadowmapProcessor::directionalShadowmap() const {
	return _momentsBasedShadowmap ? _directionalShadowmapMoments : _directionalShadowmap;
}

inline const Sampler::Pointer& ShadowmapProcessor::directionalShadowmapSampler() const {
	return _momentsBasedShadowmap ? _directionalShadowmapMomentsSampler : _directionalShadowmapSampler;
}

}
}
