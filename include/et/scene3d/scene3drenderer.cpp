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

	s3d::BaseElement::List meshes = scene.childrenOfType(s3d::ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
		mesh->prepareRenderBatches();
	
	_mainPass->begin();
	renderMeshList(_mainPass, meshes);
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
			BoundingBox transformedBox = rb->boundingBox().transform(rb->transformation());
			uint64_t key = rb->material()->sortingKey();
			if (pass->info().camera->frustum().containsBoundingBox(transformedBox))
			{
				_latestBatches[key].emplace_back(rb, transformedBox);
			}
		}
	}

	vec3 cameraPosition = pass->info().camera->position();
	for (auto& rbv : _latestBatches)
	{
		std::sort(rbv.second.begin(), rbv.second.end(), [cameraPosition](BatchFromMesh& l, BatchFromMesh& r)
		{
			const BlendState& lbs = l.batch->material()->blendState();
			const BlendState& rbs = r.batch->material()->blendState();
			float delta = (l.transformedBox.center - cameraPosition).dotSelf() - (r.transformedBox.center - cameraPosition).dotSelf();
			
			if (lbs.enabled && rbs.enabled)
				return delta >= 0.0f;
			else if (lbs.enabled)
				return true;
			else if (rbs.enabled)
				return false;
			
			return delta <= 0.0f;
		});

		for (auto& rb : rbv.second)
		{
			pass->pushRenderBatch(rb.batch);
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
