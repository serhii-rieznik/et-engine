/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/opengl/opengl_renderpass.h>
#include <et/rendering/opengl/opengl_indexbuffer.h>
#include <et/rendering/opengl/opengl.h>

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
	auto& vs = batch->vao();
	auto& prog = batch->material()->program();

	OpenGLIndexBuffer::Pointer ib = vs->indexBuffer();

	vs->bind();
	prog->bind();
	batch->material()->texture("color_texture")->bind(0);

	etDrawElements(primitiveTypeValue(ib->primitiveType()), batch->numIndexes(), 
		dataFormatValue(ib->dataFormat()), ib->indexOffset(batch->firstIndex()));
	// etDrawElements()
	// batch->makeMaterialSnapshot();
	// _renderBatches.push_back(batch);
}

}
