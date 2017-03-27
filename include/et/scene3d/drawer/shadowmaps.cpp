/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/drawer/shadowmaps.h>

namespace et
{
namespace s3d
{

void ShadowmapProcessor::setScene(const Scene::Pointer& scene, const Light::Pointer& light)
{
	_scene = scene;
	_light = light;

	Vector<BaseElement::Pointer> meshes = _scene->childrenOfType(s3d::ElementType::Mesh);

	_renderables.meshes.clear();
	_renderables.batches.clear();
	_renderables.meshes.reserve(meshes.size());
	_renderables.batches.reserve(meshes.size() * 2);
	for (Mesh::Pointer mesh : meshes)
	{
		mesh->prepareRenderBatches();
		_renderables.batches.insert(_renderables.batches.end(), mesh->renderBatches().begin(), mesh->renderBatches().end());
	}
	_renderables.activeBatches.clear();
	_renderables.activeBatches.reserve(_renderables.batches.size());
}

void ShadowmapProcessor::process(RenderInterface::Pointer& renderer, DrawerOptions& options)
{
	validate(renderer);

	for (Mesh::Pointer mesh : _renderables.meshes)
		mesh->prepareRenderBatches();

	clipAndSortRenderBatches(_renderables.batches, _renderables.activeBatches, _light, _renderables.shadowpass);

	_renderables.shadowpass->loadSharedVariablesFromCamera(_light);
	_renderables.shadowpass->loadSharedVariablesFromLight(_light);
	_renderables.shadowpass->begin({ 0, 0 });
	for (const RenderBatch::Pointer& batch : _renderables.activeBatches)
		_renderables.shadowpass->pushRenderBatch(batch);
	_renderables.shadowpass->end();
	renderer->submitRenderPass(_renderables.shadowpass);

	if (options.drawShadowmap)
	{
		vec2 vp = vector2ToFloat(renderer->rc()->size());
		vec2 sz = vec2(vp.y * _directionalShadowmap->sizeFloat(0).aspect(), vp.y);
		_renderables.debugBatch->setMaterial(_renderables.debugMaterial->instance());
		_renderables.debugBatch->setTransformation(fullscreenBatchTransform(vp, vec2(0.0f), sz));
		_renderables.debugPass->begin({ 0,0 });
		_renderables.debugPass->pushRenderBatch(_renderables.debugBatch);
		_renderables.debugPass->end();
		renderer->submitRenderPass(_renderables.debugPass);
		_renderables.debugMaterial->flushInstances();
	}
}

void ShadowmapProcessor::validate(RenderInterface::Pointer& renderer)
{
	if (_directionalShadowmap.invalid())
	{
		TextureDescription::Pointer desc(PointerInit::CreateInplace);
		desc->size = vec2i(1024, 1024);
		desc->format = TextureFormat::Depth32F;
		desc->flags = Texture::Flags::RenderTarget;
		_directionalShadowmap = renderer->createTexture(desc);
	}

	if (_renderables.shadowpass.invalid())
	{
		RenderPass::ConstructionInfo desc;
		desc.color[0].enabled = false;
		desc.depth.enabled = true;
		desc.depth.texture = _directionalShadowmap;
		desc.depth.useDefaultRenderTarget = false;
		desc.name = "depth";
		_renderables.shadowpass = renderer->allocateRenderPass(desc);
	}

	if (_renderables.debugPass.invalid())
	{
		RenderPass::ConstructionInfo desc;
		desc.color[0].loadOperation = FramebufferOperation::Load;
		desc.color[0].storeOperation = FramebufferOperation::Store;
		desc.color[0].enabled = true;
		desc.color[0].useDefaultRenderTarget = true;
		desc.depth.enabled = false;
		desc.name = "default";
		desc.priority = RenderPassPriority::UI - 1;
		_renderables.debugPass = renderer->allocateRenderPass(desc);

		_renderables.debugMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed-depth.json"));
		_renderables.debugMaterial->setTexture(MaterialTexture::BaseColor, _directionalShadowmap);
		_renderables.debugBatch = renderhelper::createFullscreenRenderBatch(_directionalShadowmap, _renderables.debugMaterial);
	}
}

}
}
