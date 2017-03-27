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

	void setScene(const Scene::Pointer& scene, const Light::Pointer& light);
	void process(RenderInterface::Pointer& renderer, DrawerOptions& options);

private:
	void validate(RenderInterface::Pointer& renderer);

private:
	Texture::Pointer _directionalShadowmap;
	Scene::Pointer _scene;
	Light::Pointer _light;

	struct Renderables
	{
		RenderPass::Pointer shadowpass;
		RenderBatchCollection batches;
		RenderBatchCollection activeBatches;
		Vector<Mesh::Pointer> meshes;

		Material::Pointer debugMaterial;
		RenderBatch::Pointer debugBatch;
		RenderPass::Pointer debugPass;
	} _renderables;
};

inline const Texture::Pointer& ShadowmapProcessor::directionalShadowmap() const {
	return _directionalShadowmap;
}

}
}
