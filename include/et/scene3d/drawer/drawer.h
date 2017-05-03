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

	enum class SupportTexture : uint32_t
	{
		Velocity,
	};

public:
	Drawer(const RenderInterface::Pointer&);

	void setRenderTarget(const Texture::Pointer&);
	void setScene(const Scene::Pointer&);
	void setEnvironmentMap(const std::string&);

	void draw();

	const Light::Pointer& directionalLight();

	const Texture::Pointer& supportTexture(SupportTexture);

private:
	void validate(RenderInterface::Pointer&);

private:
	ObjectsCache _cache;
	
	Scene::Pointer _scene;
	Vector<Mesh::Pointer> _allMeshes;

	RenderInterface::Pointer _renderer;
	CubemapProcessor::Pointer _cubemapProcessor = CubemapProcessor::Pointer(PointerInit::CreateInplace);
	ShadowmapProcessor::Pointer _shadowmapProcessor = ShadowmapProcessor::Pointer(PointerInit::CreateInplace);
	Camera::Pointer _defaultCamera = Camera::Pointer(PointerInit::CreateInplace);

	struct MainPass
	{
		RenderPass::Pointer pass;
		Texture::Pointer color;
		Texture::Pointer depth;
		Texture::Pointer velocity;
	} _main;

	struct Lighting
	{
		Light::Pointer directional = Light::Pointer::create(Light::Type::Directional);
		Material::Pointer environmentMaterial;
		RenderBatch::Pointer environmentBatch;
		std::string environmentTextureFile;
	} _lighting;
};

inline const Light::Pointer& Drawer::directionalLight()
{
	return _lighting.directional;
}

inline const Texture::Pointer& Drawer::supportTexture(SupportTexture tex)
{
	switch (tex)
	{
	case Drawer::SupportTexture::Velocity:
		return _main.velocity;

	default:
	{
		ET_ASSERT("Invalid support texture requested");
		return _renderer->blackTexture();
	}
	}
}

}
}
