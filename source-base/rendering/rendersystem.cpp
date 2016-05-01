/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et/rendering/rendersystem.h>

using namespace et;

RenderSystem::RenderSystem(RenderContext* rc) :
	_rc(rc)
{
	
}

RenderPass::Pointer RenderSystem::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	RenderPass::Pointer result = RenderPass::Pointer::create(info);
	
	return result;
}

void RenderSystem::submitRenderPass(RenderPass::Pointer pass)
{
	auto rn = _rc->renderer();
	auto& rs = _rc->renderState();
	
	for (auto& batch : pass->renderBatches())
	{
		auto& mat = batch->material().reference();
		mat.enableSnapshotInRenderState(rs, batch->materialSnapshot());
		
		auto& prog = mat.program().reference();
		prog.setTransformMatrix(batch->transformation());
        prog.setCameraProperties(pass->info().camera);
        prog.setDefaultLightPosition(pass->info().defaultLightPosition);
		
		rs.bindVertexArrayObject(batch->vao());
		rn->drawElements(batch->vao()->indexBuffer(), batch->firstIndex(), batch->numIndexes());
	}
}
