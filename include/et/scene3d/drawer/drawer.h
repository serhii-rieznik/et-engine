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

	enum : uint32_t
	{
		/*
		* State flags
		*/
		RebuildCubemap = 1 << 0,
		RebuildLookupTexture = 1 << 1,

		/*
		* Constants
		*/
		CubemapLevels = 9
	};

	DrawerOptions options;

public:
	Drawer();

	void render(RenderInterface::Pointer&, const Scene::Pointer&);

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

	void extractBatches(const Scene::Pointer&);
	void clip(RenderPass::Pointer& pass, const RenderBatchCollection&, RenderBatchInfoCollection&);
	void render(RenderPass::Pointer& pass, const RenderBatchInfoCollection&);
	void validateMainPass(RenderInterface::Pointer&, const Scene::Pointer&);
	void validateShadowPass(RenderInterface::Pointer&);
	
	void processCubemap(RenderInterface::Pointer&);
	void renderDebug(RenderInterface::Pointer&);

	void validateWrapCubemapPasses(RenderInterface::Pointer&);

private:
	ObjectsCache _cache;
	CubemapProcessor::Pointer _cubemapProcessor;
	RenderBatchCollection _renderBatches;
	RenderBatchInfoCollection _mainPassBatches;
	RenderPass::Pointer _mainPass;
    RenderBatchInfoCollection _shadowPassBatches;
	RenderPass::Pointer _shadowPass;
    Texture::Pointer _shadowTexture;

	RenderBatch::Pointer _environmentMapBatch;
	Material::Pointer _environmentMaterial;
};

}
}
