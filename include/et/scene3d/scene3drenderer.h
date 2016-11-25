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
class Renderer : public FlagsHolder
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

public:
	Renderer();

	void render(RenderInterface::Pointer&, const Scene::Pointer&);

private:
	struct RenderBatchInfo
	{
		uint64_t priority = 0;
		RenderBatch::Pointer batch;
		BoundingBox transformedBox;

		RenderBatchInfo(uint64_t p, RenderBatch::Pointer b, const BoundingBox& bb) :
			priority(p), batch(b), transformedBox(bb) { }
	};

	using RenderBatchCollection = Vector<RenderBatch::Pointer>;
	using RenderBatchInfoCollection = Vector<RenderBatchInfo>;

	void extractBatches(const Scene::Pointer&);
	void clip(RenderPass::Pointer& pass, const RenderBatchCollection&, RenderBatchInfoCollection&);
	void render(RenderPass::Pointer& pass, const RenderBatchInfoCollection&);
	void validateMainPass(RenderInterface::Pointer&, const Scene::Pointer&);
	void validateShadowPass(RenderInterface::Pointer&);

private:
	RenderBatchCollection _renderBatches;

	RenderBatchInfoCollection _mainPassBatches;
	RenderPass::Pointer _mainPass;
    RenderBatchInfoCollection _shadowPassBatches;
	RenderPass::Pointer _shadowPass;

    Texture::Pointer _shadowTexture;
};
}
}
