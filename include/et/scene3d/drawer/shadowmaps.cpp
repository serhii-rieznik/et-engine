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

	const BoundingBox::Corners& corners = _sceneBoundingBox.corners();

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

void ShadowmapProcessor::updateConfig(RenderInterface::Pointer& renderer)
{
	const std::string& shadowmapMode = renderer->options().optionValue(RenderOptions::OptionClass::ShadowMapping).name;
	_momentsBasedShadowmap = (shadowmapMode == "Moments");
}

void ShadowmapProcessor::process(RenderInterface::Pointer& renderer, DrawerOptions& options)
{
	validate(renderer);
	setupProjection(options);
	updateConfig(renderer);

	RenderPass::Pointer activePass = _momentsBasedShadowmap ? _renderables.momentsBasedShadowPass : _renderables.depthBasedShadowPass;
	activePass->loadSharedVariablesFromCamera(_light);
	activePass->loadSharedVariablesFromLight(_light);
	activePass->setSharedVariable(ObjectVariable::CameraJitter, vec4(0.0f));
	
	renderer->beginRenderPass(activePass, RenderPassBeginInfo::singlePass());
	activePass->pushImageBarrier(_directionalShadowmap, ResourceBarrier(TextureState::DepthRenderTarget));
	activePass->nextSubpass();
	for (Mesh::Pointer& mesh : _renderables.meshes)
	{
		if (_light->frustum().containsBoundingBox(mesh->tranformedBoundingBox()))
		{
			const mat4& transform = mesh->transform();
			const mat4& rotationTransform = mesh->rotationTransform();
			activePass->setSharedVariable(ObjectVariable::WorldTransform, transform);
			activePass->setSharedVariable(ObjectVariable::WorldRotationTransform, rotationTransform);
			for (const RenderBatch::Pointer& batch : mesh->renderBatches())
				activePass->pushRenderBatch(batch);
		}
	}
	activePass->endSubpass();
	activePass->pushImageBarrier(_directionalShadowmap, ResourceBarrier(TextureState::ShaderResource));
	renderer->submitRenderPass(activePass);

	if (_momentsBasedShadowmap)
	{
		renderer->beginRenderPass(_renderables.blurPass0, RenderPassBeginInfo::singlePass());
		_renderables.blurPass0->nextSubpass();
		_renderables.blurPass0->pushRenderBatch(_renderables.blurBatch0);
		_renderables.blurPass0->endSubpass();
		renderer->submitRenderPass(_renderables.blurPass0);

		renderer->beginRenderPass(_renderables.blurPass1, RenderPassBeginInfo::singlePass());
		_renderables.blurPass1->nextSubpass();
		_renderables.blurPass1->pushRenderBatch(_renderables.blurBatch1);
		_renderables.blurPass1->endSubpass();
		renderer->submitRenderPass(_renderables.blurPass1);
	}

	if (options.drawShadowmap)
	{
		vec2 vp = vector2ToFloat(renderer->contextSize());
		vec2 depthSz = vec2(0.5f * vp.x);
		vec2 colorSz = depthSz;
		vec2 depthPt = vec2(0.0f, vp.y - depthSz.y);
		vec2 colorPt = vec2(depthSz.x, depthPt.y);

		renderer->beginRenderPass(_renderables.debugPass, RenderPassBeginInfo::singlePass());
		_renderables.debugPass->nextSubpass();
		_renderables.debugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, depthPt, depthSz));
		_renderables.debugPass->pushRenderBatch(_renderables.debugDepthBatch);
		_renderables.debugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, colorPt, colorSz));
		_renderables.debugPass->pushRenderBatch(_renderables.debugColorBatch);
		_renderables.debugPass->endSubpass();
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
		desc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget;
		_directionalShadowmap = renderer->createTexture(desc);

		desc->format = TextureFormat::RGBA16;
		_directionalShadowmapMoments = renderer->createTexture(desc);
		_directionalShadowmapMomentsBuffer = renderer->createTexture(desc);
	}

	{
		Sampler::Description shadowSamplerDesc;
		shadowSamplerDesc.wrapU = TextureWrap::ClampToEdge;
		shadowSamplerDesc.wrapV = TextureWrap::ClampToEdge;
		shadowSamplerDesc.wrapW = TextureWrap::ClampToEdge;
		shadowSamplerDesc.minFilter = TextureFiltration::Linear;
		shadowSamplerDesc.magFilter = TextureFiltration::Linear;
		shadowSamplerDesc.mipFilter = TextureFiltration::Nearest;
		shadowSamplerDesc.maxAnisotropy = 16.0f;
		shadowSamplerDesc.compareEnabled = true;
		shadowSamplerDesc.compareFunction = CompareFunction::LessOrEqual;
		_directionalShadowmapSampler = renderer->createSampler(shadowSamplerDesc);

		shadowSamplerDesc.compareEnabled = false;
		_directionalShadowmapMomentsSampler = renderer->createSampler(shadowSamplerDesc);
	}

	{
		RenderPass::ConstructionInfo desc("depth");
		desc.depth.texture = _directionalShadowmap;
		desc.depth.loadOperation = FramebufferOperation::Clear;
		desc.depth.storeOperation = FramebufferOperation::Store;
		desc.depth.targetClass = RenderTarget::Class::Texture;
		_renderables.depthBasedShadowPass = renderer->allocateRenderPass(desc);

		desc.color[0].texture = _directionalShadowmapMoments;
		desc.color[0].loadOperation = FramebufferOperation::Clear;
		desc.color[0].storeOperation = FramebufferOperation::Store;
		desc.color[0].clearValue = vec4(std::numeric_limits<float>::max());
		desc.color[0].targetClass = RenderTarget::Class::Texture;
		_renderables.momentsBasedShadowPass = renderer->allocateRenderPass(desc);
	}

	{
		RenderPass::ConstructionInfo desc("default");
		desc.color[0].loadOperation = FramebufferOperation::Load;
		desc.color[0].storeOperation = FramebufferOperation::Store;
		desc.color[0].targetClass = RenderTarget::Class::DefaultBuffer;
		desc.depth.targetClass = RenderTarget::Class::Disabled;
		desc.priority = RenderPassPriority::UI - 1;
		_renderables.debugPass = renderer->allocateRenderPass(desc);

		Material::Pointer mtl = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed-depth.json"));
		mtl->setTexture(MaterialTexture::Input, _directionalShadowmap);
		_renderables.debugDepthBatch = renderhelper::createQuadBatch(MaterialTexture::Input, _directionalShadowmap, mtl, renderhelper::QuadType::Default);

		mtl = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed.json"));
		mtl->setTexture(MaterialTexture::Input, _directionalShadowmap);
		_renderables.debugColorBatch = renderhelper::createQuadBatch(MaterialTexture::Input, _directionalShadowmapMoments, mtl, renderhelper::QuadType::Default);
	}

	{
		RenderPass::ConstructionInfo desc("gaussian-blur");
		desc.color[0].texture = _directionalShadowmapMomentsBuffer;
		desc.color[0].loadOperation = FramebufferOperation::DontCare;
		desc.color[0].storeOperation = FramebufferOperation::Store;
		desc.color[0].targetClass = RenderTarget::Class::Texture;
		_renderables.blurPass0 = renderer->allocateRenderPass(desc);
		
		desc.color[0].texture = _directionalShadowmapMoments;
		_renderables.blurPass1 = renderer->allocateRenderPass(desc);

		Material::Pointer mtl = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/image-processing.json"));
		_renderables.blurBatch0 = renderhelper::createQuadBatch(MaterialTexture::Input, _directionalShadowmapMoments, mtl);
		_renderables.blurBatch0->material()->setVector(MaterialVariable::ExtraParameters, vec4(1.0f, 0.0f, 0.0f, 0.0f));
		
		_renderables.blurBatch1 = renderhelper::createQuadBatch(MaterialTexture::Input, _directionalShadowmapMomentsBuffer, mtl);
		_renderables.blurBatch1->material()->setVector(MaterialVariable::ExtraParameters, vec4(0.0f, 1.0f, 0.0f, 0.0f));
	}
}

}
}
