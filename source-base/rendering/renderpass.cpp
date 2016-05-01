/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/renderpass.h>

using namespace et;

RenderPass::RenderPass(const ConstructionInfo& info) :
	_info(info)
{
	_renderBatches.reserve(256);
}

RenderPass::~RenderPass()
{
	for (auto& batch : _renderBatches)
	{
		batch->material()->clearSnapshots();
	}
}

void RenderPass::pushRenderBatch(RenderBatch::Pointer batch)
{
	batch->makeMaterialSnapshot();
	_renderBatches.push_back(batch);
}

const Vector<RenderBatch::Pointer>& RenderPass::renderBatches() const
{
	return _renderBatches;
}

Vector<RenderBatch::Pointer>& RenderPass::renderBatches()
{
	return _renderBatches;
}
