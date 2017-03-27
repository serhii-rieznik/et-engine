/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/drawer/common.h>

namespace et
{
namespace s3d
{

mat4 fullscreenBatchTransform(const vec2& viewport, const vec2& origin, const vec2& size)
{
	vec2 fsz = size / viewport;
	vec2 fps = origin / viewport;
	mat4 result = scaleMatrix(fsz.x, fsz.y, 1.0f);
	result[3].x = 2.0f * (fps.x + 0.5f * fsz.x) - 1.0f;
	result[3].y = 2.0f * (fps.y + 0.5f * fsz.y) - 1.0f;
	return result;
}

void clipAndSortRenderBatches(const RenderBatchCollection& batchesIn, RenderBatchCollection& batchesOut,
	const Camera::Pointer& camera, const RenderPass::Pointer& pass)
{
	batchesOut.clear();
	batchesOut.reserve(batchesIn.size());

	for (const RenderBatch::Pointer& batch : batchesIn)
	{
		// TODO : frustum clipping
		batchesOut.emplace_back(batch);
	}

	vec3 cameraPosition = camera->position();
	auto cmp = [cameraPosition, pass](RenderBatch::Pointer l, RenderBatch::Pointer r)
	{
		const BlendState& lbs = l->material()->configuration(pass->info().name).blendState;
		const BlendState& rbs = r->material()->configuration(pass->info().name).blendState;
		if (lbs.enabled == rbs.enabled)
		{
			float delta = (l->transformedBoundingBox().center - cameraPosition).dotSelf() -
				(r->transformedBoundingBox().center - cameraPosition).dotSelf();
			return lbs.enabled ? (delta >= 0.0f) : (delta < 0.0f);
		}
		return static_cast<int>(lbs.enabled) < static_cast<int>(rbs.enabled);
	};

	std::stable_sort(batchesOut.begin(), batchesOut.end(), cmp);
}

}
}