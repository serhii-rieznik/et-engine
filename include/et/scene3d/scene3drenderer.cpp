/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/scene3drenderer.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/base/primitives.h>

using namespace et;
using namespace et::s3d;

s3d::Renderer::Renderer() :
	FlagsHolder(RenderAll)
{
	
}

void s3d::Renderer::render(RenderInterface::Pointer renderer, const Scene& scene, Camera::Pointer camera)
{
	if (_mainPass.invalid())
	{
		Camera::Pointer lightCamera;
		auto lights = scene.childrenOfType(et::s3d::ElementType::Light);
		if (!lights.empty())
		{
			lightCamera = static_cast<s3d::Light::Pointer>(lights.front())->camera();
		}

		RenderPass::ConstructionInfo passInfo;
		passInfo.target.colorLoadOperation = et::FramebufferOperation::Clear;
		passInfo.target.depthLoadOperation = et::FramebufferOperation::Clear;
		passInfo.target.clearColor = vec4(0.25f, 0.3333f, 0.5f, 1.0f);
		passInfo.camera = camera;
		passInfo.light = lightCamera;
		_mainPass = renderer->allocateRenderPass(passInfo);
	}

	_mainPass->begin();
	renderMeshList(_mainPass, scene.childrenOfType(s3d::ElementType::Mesh));
	_mainPass->end();

	renderer->submitRenderPass(_mainPass);
}

void s3d::Renderer::renderMeshList(RenderPass::Pointer pass, const s3d::BaseElement::List& meshes)
{
	for (auto& lb : _latestBatches)
	{
		lb.second.clear();
	}
	
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		mesh->prepareRenderBatches();
		for (auto& rb : mesh->renderBatches())
		{
			uint64_t key = rb->material()->sortingKey();
			_latestBatches[key].emplace_back(rb, mesh);
		}
	}

	auto cameraPosition = pass->info().camera->position();
	for (auto& rbv : _latestBatches)
	{
		std::random_shuffle(rbv.second.begin(), rbv.second.end());

		/*
		std::sort(rbv.second.begin(), rbv.second.end(), [cameraPosition](BatchFromMesh& l, BatchFromMesh& r)
		{
			auto lip = l.first->transformation() * l.first->boundingBox().center;
			auto rip = r.first->transformation() * r.first->boundingBox().center;
			if (l.first->material()->blendState().enabled)
				return (lip - cameraPosition).dotSelf() > (rip - cameraPosition).dotSelf();
			else
				return (lip - cameraPosition).dotSelf() < (rip - cameraPosition).dotSelf();
		});
		// */
		
		for (auto& rb : rbv.second)
		{
			pass->pushRenderBatch(rb.first);
		}
	}
}

void s3d::Renderer::renderTransformedBoundingBox(RenderPass::Pointer pass, const BoundingBox& b, const mat4& t)
{
    if (hasFlag(RenderDebugObjects))
    {
        // _bboxBatch->material()->setProperty("bboxScale", b.halfDimension);
        // _bboxBatch->material()->setProperty("bboxCenter", b.center);
        _bboxBatch->setTransformation(t);
        pass->pushRenderBatch(_bboxBatch);
    }
}
