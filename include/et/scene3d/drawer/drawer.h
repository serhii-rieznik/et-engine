/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/drawer/shadowmaps.h>
#include <et/scene3d/drawer/cubemaps.h>

namespace et
{
namespace s3d
{
class Drawer : public Shared, public FlagsHolder
{
public:
	ET_DECLARE_POINTER(Drawer);

	DrawerOptions options;

public:
	Drawer(const RenderInterface::Pointer&);

	void setRenderTarget(const Texture::Pointer&);
	void setScene(const Scene::Pointer&);
	void setEnvironmentMap(const std::string&);

	void draw();

	Light::Pointer directionalLight();

private:
	void validate(RenderInterface::Pointer&);

private:
	ObjectsCache _cache;
	Scene::Pointer _scene;
	RenderInterface::Pointer _renderer;
	CubemapProcessor::Pointer _cubemapProcessor = CubemapProcessor::Pointer(PointerInit::CreateInplace);
	ShadowmapProcessor::Pointer _shadowmapProcessor = ShadowmapProcessor::Pointer(PointerInit::CreateInplace);
	Camera::Pointer _defaultCamera = Camera::Pointer(PointerInit::CreateInplace);

	struct MainPass
	{
		RenderPass::Pointer pass;
		RenderBatchCollection all;
		RenderBatchCollection rendereable;
		Texture::Pointer renderTarget;
	} _main;

	struct Lighting
	{
		Light::Pointer directional = Light::Pointer::create(Light::Type::Directional);
		Material::Pointer environmentMaterial;
		RenderBatch::Pointer environmentBatch;
		std::string environmentTextureFile;
	} _lighting;
};

inline Light::Pointer Drawer::directionalLight()
{
	return _lighting.directional;
}

}
}
