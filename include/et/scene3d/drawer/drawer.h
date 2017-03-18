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

	void draw();
	void setScene(const Scene::Pointer&);
	void setEnvironmentMap(const std::string&);

private:
	struct RenderBatchInfo
	{
		uint64_t priority = 0;
		RenderBatch::Pointer batch;
		BoundingBox transformedBox;

		RenderBatchInfo(const RenderBatch::Pointer& b) : 
			batch(b) { }

		RenderBatchInfo(uint64_t p, const RenderBatch::Pointer& b, const BoundingBox& bb) :
			priority(p), batch(b), transformedBox(bb) { }
	};

	using RenderBatchCollection = Vector<RenderBatch::Pointer>;
	using RenderBatchInfoCollection = Vector<RenderBatchInfo>;

	void clip(const Camera::Pointer&, const RenderBatchCollection&, RenderBatchInfoCollection&);
	void validate(RenderInterface::Pointer&);
	void renderDebug(RenderInterface::Pointer&);

private:
	ObjectsCache _cache;
	RenderInterface::Pointer _renderer;
	CubemapProcessor::Pointer _cubemapProcessor;

	struct MainPass
	{
		RenderPass::Pointer pass;
		RenderBatchCollection rendereables;
		RenderBatchInfoCollection batches;
		Camera::Pointer camera = Camera::Pointer::create();
	} _main;

	struct Lighting
	{
		Light::Pointer directional = Light::Pointer::create(Light::Type::Directional);
		Material::Pointer environmentMaterial;
		RenderBatch::Pointer environmentBatch;
		std::string environmentTextureFile;
	} _lighting;

};

}
}
