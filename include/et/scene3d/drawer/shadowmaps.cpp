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
}

void ShadowmapProcessor::setupProjection(DrawerOptions& options)
{
	vec3 d = -_light->direction();
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

		float left = viewMin.x - projectionOffset;
		float right = viewMax.x + projectionOffset;
		float top = viewMax.y + projectionOffset;
		float bottom = viewMin.y - projectionOffset;

		float hCenter = 0.5f * (right + left);
		float hSize = 0.5f * (right - left) * options.shadowmapScale;
		left = hCenter - hSize;
		right = hCenter + hSize;
		
		float vCenter = 0.5f * (bottom + top);
		float vSize = 0.5f * (top - bottom) * options.shadowmapScale;
		bottom = vCenter - vSize;
		top = vCenter + vSize;

		lightCamera.orthogonalProjection(left, right, bottom, top,
			-(viewMax.z + projectionOffset), -(viewMin.z - projectionOffset));
	}
	_light->setViewMatrix(lightCamera.viewMatrix());
	_light->setProjectionMatrix(lightCamera.projectionMatrix());
}

void ShadowmapProcessor::process(RenderInterface::Pointer& renderer, DrawerOptions& options)
{
	validate(renderer);
	setupProjection(options);

	_renderables.shadowpass->loadSharedVariablesFromCamera(_light);
	_renderables.shadowpass->loadSharedVariablesFromLight(_light);
	
	_renderables.shadowpass->begin(RenderPassBeginInfo::singlePass());
	_renderables.shadowpass->pushImageBarrier(_directionalShadowmap, ResourceBarrier(TextureState::DepthRenderTarget));
	_renderables.shadowpass->nextSubpass();
	for (Mesh::Pointer& mesh : _renderables.meshes)
	{
		const mat4& transform = mesh->transform();
		const mat4& rotationTransform = mesh->rotationTransform();
		_renderables.shadowpass->setSharedVariable(ObjectVariable::WorldTransform, transform);
		_renderables.shadowpass->setSharedVariable(ObjectVariable::WorldRotationTransform, rotationTransform);
		for (const RenderBatch::Pointer& batch : mesh->renderBatches())
			_renderables.shadowpass->pushRenderBatch(batch);
	}
	_renderables.shadowpass->endSubpass();

	_renderables.shadowpass->pushImageBarrier(_directionalShadowmap, ResourceBarrier(TextureState::ShaderResource));
	_renderables.shadowpass->end();
	renderer->submitRenderPass(_renderables.shadowpass);

	_renderables.blurPass0->begin(RenderPassBeginInfo::singlePass());
	_renderables.blurPass0->nextSubpass();
	_renderables.blurPass0->pushRenderBatch(_renderables.blurBatch0);
	_renderables.blurPass0->endSubpass();
	_renderables.blurPass0->end();
	renderer->submitRenderPass(_renderables.blurPass0);

	_renderables.blurPass1->begin(RenderPassBeginInfo::singlePass());
	_renderables.blurPass1->nextSubpass();
	_renderables.blurPass1->pushRenderBatch(_renderables.blurBatch1);
	_renderables.blurPass1->endSubpass();
	_renderables.blurPass1->end();
	renderer->submitRenderPass(_renderables.blurPass1);

	if (options.drawShadowmap)
	{
		vec2 vp = vector2ToFloat(renderer->rc()->size());
		vec2 depthSz = vec2(0.5f * vp.x);
		vec2 colorSz = depthSz;
		vec2 depthPt = vec2(0.0f, vp.y - depthSz.y);
		vec2 colorPt = vec2(depthSz.x, depthPt.y);

		_renderables.debugPass->begin(RenderPassBeginInfo::singlePass());
		_renderables.debugPass->nextSubpass();
		_renderables.debugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, depthPt, depthSz));
		_renderables.debugPass->pushRenderBatch(_renderables.debugDepthBatch);
		
		_renderables.debugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, colorPt, colorSz));
		_renderables.debugPass->pushRenderBatch(_renderables.debugColorBatch);
		_renderables.debugPass->endSubpass();
		_renderables.debugPass->end();
		
		renderer->submitRenderPass(_renderables.debugPass);
	}
}

void ShadowmapProcessor::validate(RenderInterface::Pointer& renderer)
{
	if (_renderables.initialized)
		return;

	_renderables.initialized = true;
	{
		TextureDescription::Pointer desc(PointerInit::CreateInplace);
		desc->size = vec2i(2048);
		desc->format = TextureFormat::Depth32F;
		desc->flags = Texture::Flags::RenderTarget;
		_directionalShadowmap = renderer->createTexture(desc);

		desc->format = TextureFormat::RGBA16;
		_directionalShadowmapMoments = renderer->createTexture(desc);
		_directionalShadowmapMomentsBuffer = renderer->createTexture(desc);
	}

	{
		RenderPass::ConstructionInfo desc;
		desc.color[0].texture = _directionalShadowmapMoments;
		desc.color[0].loadOperation = FramebufferOperation::Clear;
		desc.color[0].storeOperation = FramebufferOperation::Store;
		desc.color[0].clearValue = vec4(std::numeric_limits<float>::max());
		desc.color[0].useDefaultRenderTarget = false;
		desc.color[0].enabled = true;
		
		desc.depth.texture = _directionalShadowmap;
		desc.depth.loadOperation = FramebufferOperation::Clear;
		desc.depth.storeOperation = FramebufferOperation::Store;
		desc.depth.useDefaultRenderTarget = false;
		desc.depth.enabled = true;
		
		desc.name = "depth";
		_renderables.shadowpass = renderer->allocateRenderPass(desc);
	}

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

		Material::Pointer mtl = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed-depth.json"));
		mtl->setTexture(MaterialTexture::BaseColor, _directionalShadowmap);
		_renderables.debugDepthBatch = renderhelper::createFullscreenRenderBatch(_directionalShadowmap, mtl);

		mtl = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed.json"));
		mtl->setTexture(MaterialTexture::BaseColor, _directionalShadowmap);
		_renderables.debugColorBatch = renderhelper::createFullscreenRenderBatch(_directionalShadowmapMoments, mtl);
	}

	{
		RenderPass::ConstructionInfo desc;
		desc.color[0].texture = _directionalShadowmapMomentsBuffer;
		desc.color[0].loadOperation = FramebufferOperation::DontCare;
		desc.color[0].storeOperation = FramebufferOperation::Store;
		desc.color[0].useDefaultRenderTarget = false;
		desc.color[0].enabled = true;
		desc.name = "gaussian-blur";
		_renderables.blurPass0 = renderer->allocateRenderPass(desc);
		
		desc.color[0].texture = _directionalShadowmapMoments;
		_renderables.blurPass1 = renderer->allocateRenderPass(desc);

		Material::Pointer mtl = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/image-processing.json"));
		_renderables.blurBatch0 = renderhelper::createFullscreenRenderBatch(_directionalShadowmapMoments, mtl);
		_renderables.blurBatch0->material()->setVector(MaterialVariable::ExtraParameters, vec4(1.0f, 0.0f, 0.0f, 0.0f));
		
		_renderables.blurBatch1 = renderhelper::createFullscreenRenderBatch(_directionalShadowmapMomentsBuffer, mtl);
		_renderables.blurBatch1->material()->setVector(MaterialVariable::ExtraParameters, vec4(0.0f, 1.0f, 0.0f, 0.0f));
	}
}

}
}
