/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/opengl/opengl_renderpass.h>

namespace et
{

OpenGLRenderPass::OpenGLRenderPass(const RenderPass::ConstructionInfo& info) :
	RenderPass(info)
{
	_renderBatches.reserve(256);
}

OpenGLRenderPass::~OpenGLRenderPass()
{
	for (auto& batch : _renderBatches)
	{
		batch->material()->clearSnapshots();
	}
}

void OpenGLRenderPass::pushRenderBatch(RenderBatch::Pointer batch)
{
	batch->makeMaterialSnapshot();
	_renderBatches.push_back(batch);
}

}
