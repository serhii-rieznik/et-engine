/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/renderer.h>
#include <et/scene3d/scene3d.h>

namespace et
{
namespace s3d
{
class Renderer : public Shared, public FlagsHolder
{
public:
	ET_DECLARE_POINTER(Renderer);

	enum : uint64_t
	{
		RenderMeshes = 0x01,
		RenderDebugObjects = 0x02,
		Wireframe = 0x04,

		RenderAll = RenderMeshes | RenderDebugObjects
	};

	struct Options
	{
		bool drawEnvironmentProbe = false;
	} options;

public:
	Renderer();

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
	void renderDebug(RenderInterface::Pointer&);

	void validateWrapCubemapPass(RenderInterface::Pointer&);

	mat4 fullscreenBatchTransform(const vec2& viewport, const vec2& origin, const vec2& size);

private:
	enum : uint32_t
	{
		RebuildCubemap = 1 << 0
	};

private:
	ObjectsCache _cache;
	RenderBatchCollection _renderBatches;

	RenderBatchInfoCollection _mainPassBatches;
	RenderPass::Pointer _mainPass;
    RenderBatchInfoCollection _shadowPassBatches;
	RenderPass::Pointer _shadowPass;

	Material::Pointer _cubemapMaterial;
	RenderPass::Pointer _wrapCubemapPass;
	RenderBatch::Pointer _wrapCubemapBatch;
	
	RenderPass::Pointer _cubemapDebugPass;
	RenderBatch::Pointer _cubemapDebugBatch;

	Sampler::Pointer _cubemapDebugSampler;
    Texture::Pointer _shadowTexture;
	Texture::Pointer _envTexture;
	Texture::Pointer _envCubemap;
	Material::Pointer _envMaterial;
	RenderBatch::Pointer _envBatch;

	uint32_t _state = 0;
};
}
}
