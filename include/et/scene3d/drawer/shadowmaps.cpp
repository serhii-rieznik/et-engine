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

void ShadowmapProcessor::setScene(const Scene::Pointer& scene, Light::Pointer& light)
{
	_scene = scene;

	Vector<BaseElement::Pointer> meshes = _scene->childrenOfType(s3d::ElementType::Mesh);

	_renderables.meshes.clear();
	_renderables.meshes.reserve(meshes.size());

	vec3 minVertex(std::numeric_limits<float>::max());
	vec3 maxVertex = -minVertex;
	for (Mesh::Pointer mesh : meshes)
	{
		minVertex = minv(minVertex, mesh->tranformedBoundingBox().minVertex());
		maxVertex = maxv(maxVertex, mesh->tranformedBoundingBox().maxVertex());
		_renderables.meshes.emplace_back(mesh);
	}
	_sceneBoundingBox = BoundingBox(0.5f * (maxVertex + minVertex), 0.5f * (maxVertex - minVertex));
	updateLight(light);
}

void ShadowmapProcessor::updateLight(Light::Pointer& light)
{
	_light = light;

	vec3 d = -light->direction();
	vec3 s = normalize(cross(unitY, d));
	vec3 u = normalize(cross(d, s));

	mat3 lightViewMatrix;
	lightViewMatrix[0][0] = s.x; lightViewMatrix[0][1] = u.x; lightViewMatrix[0][2] = -d.x;
	lightViewMatrix[1][0] = s.y; lightViewMatrix[1][1] = u.y; lightViewMatrix[1][2] = -d.y;
	lightViewMatrix[2][0] = s.z; lightViewMatrix[2][1] = u.z; lightViewMatrix[2][2] = -d.z;

	BoundingBox::Corners corners;
	_sceneBoundingBox.calculateCorners(corners);

	vec3 viewMin(std::numeric_limits<float>::max());
	vec3 viewMax = -viewMin;
	for (const vec3& c : corners)
	{
		vec3 t = lightViewMatrix * c;
		viewMin = minv(viewMin, t);
		viewMax = maxv(viewMax, t);
	}
	
	Camera lightCamera;
	{
		float projectionOffset = 0.05f * length(viewMax - viewMin);
		lightCamera.setViewMatrix(mat4(lightViewMatrix));
		lightCamera.orthogonalProjection(
			viewMin.x - projectionOffset, viewMax.x + projectionOffset,
			viewMin.y - projectionOffset, viewMax.y + projectionOffset,
			-(viewMax.z + projectionOffset), -(viewMin.z - projectionOffset));
	}
	light->setViewMatrix(lightCamera.viewMatrix());
	light->setProjectionMatrix(lightCamera.projectionMatrix());
}

void ShadowmapProcessor::process(RenderInterface::Pointer& renderer, DrawerOptions& options)
{
	validate(renderer);

	_renderables.shadowpass->loadSharedVariablesFromCamera(_light);
	_renderables.shadowpass->loadSharedVariablesFromLight(_light);
	
	_renderables.shadowpass->begin(RenderPassBeginInfo::singlePass());
	_renderables.shadowpass->pushImageBarrier(_directionalShadowmap, ResourceBarrier(TextureState::DepthRenderTarget));
	_renderables.shadowpass->nextSubpass();
	for (Mesh::Pointer mesh : _renderables.meshes)
	{
		const mat4& transform = mesh->transform();
		const mat4& rotationTransform = mesh->rotationTransform();
		_renderables.shadowpass->setSharedVariable(ObjectVariable::WorldTransform, transform);
		_renderables.shadowpass->setSharedVariable(ObjectVariable::WorldRotationTransform, rotationTransform);
		for (const RenderBatch::Pointer& batch : mesh->renderBatches())
			_renderables.shadowpass->pushRenderBatch(batch);
	}
	_renderables.shadowpass->endSubpass();
	_renderables.shadowpass->end();
	_renderables.shadowpass->pushImageBarrier(_directionalShadowmap, ResourceBarrier(TextureState::ShaderResource));
	renderer->submitRenderPass(_renderables.shadowpass);

	if (options.drawShadowmap)
	{
		vec2 vp = vector2ToFloat(renderer->rc()->size());
		vec2 sz = vec2(vp.y * _directionalShadowmap->sizeFloat(0).aspect(), vp.y);
		_renderables.debugBatch->setMaterial(_renderables.debugMaterial->instance());
		
		_renderables.debugPass->begin(RenderPassBeginInfo::singlePass());
		_renderables.debugPass->nextSubpass();
		_renderables.debugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, vec2(0.0f), sz));
		_renderables.debugPass->pushRenderBatch(_renderables.debugBatch);
		_renderables.debugPass->endSubpass();
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
		desc.depth.loadOperation = FramebufferOperation::Clear;
		desc.depth.storeOperation = FramebufferOperation::Store;
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
