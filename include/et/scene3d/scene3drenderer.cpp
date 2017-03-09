/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/scene3drenderer.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/base/primitives.h>
#include <et/rendering/base/helpers.h>

namespace et
{
namespace s3d
{

Renderer::Renderer() :
	FlagsHolder(RenderAll)
{
}

void Renderer::render(RenderInterface::Pointer& renderer, const Scene::Pointer& scene)
{
	extractBatches(scene);

	validateMainPass(renderer, scene);
	validateShadowPass(renderer);
	validateWrapCubemapPass(renderer);

	if (_envMaterial.invalid())
	{
		_envMaterial = renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::EnvironmentMap);
	}

	if (_shadowPass.valid())
	{
		clip(_shadowPass, _renderBatches, _shadowPassBatches);
		render(_shadowPass, _shadowPassBatches);
		renderer->submitRenderPass(_shadowPass);
	}

	if (_state & RebuildCubemap)
	{
		_wrapCubemapBatch->material()->setTexture(MaterialTexture::BaseColor, _envTexture);
		_wrapCubemapPass->executeSingleRenderBatch(_wrapCubemapBatch);
		renderer->submitRenderPass(_wrapCubemapPass);
		_state &= ~RebuildCubemap;
	}
	else
	{
		printf(".");
	}

	clip(_mainPass, _renderBatches, _mainPassBatches);

	if (_envBatch.valid())
		_mainPassBatches.emplace_back(_envBatch);

	render(_mainPass, _mainPassBatches);
	renderer->submitRenderPass(_mainPass);

	_cubemapDebugPass->executeSingleRenderBatch(_cubemapDebugBatch);
	renderer->submitRenderPass(_cubemapDebugPass);
}

void Renderer::validateMainPass(RenderInterface::Pointer& renderer, const Scene::Pointer& scene)
{
	Light::Pointer directionalLight;
	Light::Pointer environmentLight;

	BaseElement::List lights = scene->childrenOfType(ElementType::Light);
	if (lights.size() > 0)
	{
		for (LightElement::Pointer le : lights)
		{
			if (le->light()->type() == Light::Type::Directional)
				directionalLight = le->light();
			else if (le->light()->type() == Light::Type::ImageBasedSky)
				environmentLight = le->light();
		}
	}

	bool mainPassValid = _mainPass.valid();
	bool sameCamera = mainPassValid ? (_mainPass->info().camera == scene->mainCamera()) : true;
	bool sameDirectionalLight = mainPassValid ? (_mainPass->info().light == directionalLight) : true;
	bool sameEnvLight = (environmentLight.valid() && _envTexture.valid()) ? (environmentLight->environmentMap() == _envTexture->origin()) : true;

	if (mainPassValid && sameCamera && sameDirectionalLight && sameEnvLight)
		return;

	_envTexture = renderer->blackTexture();
	if (environmentLight.valid() && (environmentLight->type() == Light::Type::ImageBasedSky))
	{
		_envTexture = renderer->loadTexture(environmentLight->environmentMap(), _cache);
		_envBatch = renderhelper::createFullscreenRenderBatch(_envTexture, _envMaterial);
	}
	else
	{
		_envBatch.reset(nullptr);
	}
	
	_state |= RebuildCubemap;

	RenderPass::ConstructionInfo passInfo;
	passInfo.camera = scene->mainCamera();
	passInfo.light = directionalLight;
	passInfo.color[0].loadOperation = FramebufferOperation::Clear;
	passInfo.color[0].storeOperation = FramebufferOperation::Store;
	passInfo.color[0].enabled = true;
	passInfo.color[0].clearValue = vec4(0.333333f, 0.333333f, 0.333333f, 1.0f);
	passInfo.depth.loadOperation = FramebufferOperation::Clear;
	passInfo.depth.storeOperation = FramebufferOperation::DontCare;
	passInfo.depth.enabled = true;
	passInfo.name = RenderPass::kPassNameForward;

	_mainPass = renderer->allocateRenderPass(passInfo);
	_mainPass->setSharedTexture(MaterialTexture::Environment, _envTexture, renderer->defaultSampler());

	_shadowTexture.reset(nullptr);
	_cache.flush();
}

void Renderer::validateShadowPass(RenderInterface::Pointer& renderer)
{
	if (_shadowTexture.invalid())
	{
		TextureDescription::Pointer desc = TextureDescription::Pointer::create();
		desc->isRenderTarget = true;
		desc->size = vec2i(2048); // TODO : variable size
		desc->format = TextureFormat::Depth32F;
		_shadowTexture = renderer->createTexture(desc);

		Sampler::Description smpDesc;
		smpDesc.minFilter = TextureFiltration::Linear;
		smpDesc.magFilter = TextureFiltration::Linear;
		smpDesc.wrapU = TextureWrap::ClampToEdge;
		smpDesc.wrapV = TextureWrap::ClampToEdge;
		Sampler::Pointer smp = renderer->createSampler(smpDesc);

		_mainPass->setSharedTexture(MaterialTexture::Shadow, _shadowTexture, smp);
	}

	if (_mainPass->info().light.valid() && (_shadowPass.invalid() || (_shadowPass->info().camera != _mainPass->info().light)))
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.camera = _mainPass->info().light;
		passInfo.light = _mainPass->info().light;
		passInfo.priority = RenderPassPriority::Default - 1;
		passInfo.depth.texture = _shadowTexture;
		passInfo.depth.loadOperation = FramebufferOperation::Clear;
		passInfo.depth.storeOperation = FramebufferOperation::Store;
		passInfo.depth.useDefaultRenderTarget = false;
		passInfo.depth.enabled = true;
		passInfo.color[0].enabled = false;
		passInfo.depthBias = 1.0f;
		passInfo.depthSlope = 1.0f;
		passInfo.name = RenderPass::kPassNameDepth;
		_shadowPass = renderer->allocateRenderPass(passInfo);
	}
}

void Renderer::extractBatches(const Scene::Pointer& scene)
{
	BaseElement::List meshes = scene->childrenOfType(ElementType::Mesh);

	_renderBatches.clear();
	_renderBatches.reserve(2 * meshes.size());
	for (Mesh::Pointer mesh : meshes)
	{
		mesh->prepareRenderBatches();
		_renderBatches.insert(_renderBatches.end(), mesh->renderBatches().begin(), mesh->renderBatches().end());
	}
}

void Renderer::clip(RenderPass::Pointer& pass, const RenderBatchCollection& inBatches, RenderBatchInfoCollection& passBatches)
{
	passBatches.clear();
	passBatches.reserve(inBatches.size());

	for (const RenderBatch::Pointer& batch : inBatches)
	{
		BoundingBox transformedBox = batch->boundingBox().transform(batch->transformation());
		if (1 || pass->info().camera->frustum().containsBoundingBox(transformedBox))
		{
			uint64_t key = batch->material()->sortingKey();
			passBatches.emplace_back(key, batch, transformedBox);
		}
	}

	vec3 cameraPosition = pass->info().camera->position();
	auto cmp = [cameraPosition, &pass](const RenderBatchInfo& l, const RenderBatchInfo& r)
	{
		if (l.priority != r.priority)
			return l.priority > r.priority;

		const BlendState& lbs = l.batch->material()->configuration(pass->info().name).blendState;
		const BlendState& rbs = r.batch->material()->configuration(pass->info().name).blendState;
		if (lbs.enabled == rbs.enabled)
		{
			float delta = (l.transformedBox.center - cameraPosition).dotSelf() - (r.transformedBox.center - cameraPosition).dotSelf();
			return lbs.enabled ? (delta >= 0.0f) : (delta < 0.0f);
		}
		return static_cast<int>(lbs.enabled) < static_cast<int>(rbs.enabled);
	};

	std::stable_sort(passBatches.begin(), passBatches.end(), cmp);
}

void Renderer::render(RenderPass::Pointer& pass, const RenderBatchInfoCollection& batches)
{
	pass->begin();
	for (const RenderBatchInfo& rb : batches)
		pass->pushRenderBatch(rb.batch);
	pass->end();
}

void Renderer::validateWrapCubemapPass(RenderInterface::Pointer& renderer)
{
	if (_cubemapMaterial.invalid())
	{
		_cubemapMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap.json"));
	}

	if (_envCubemap.invalid())
	{
		TextureDescription::Pointer cubemapDesc(PointerInit::CreateInplace);
		cubemapDesc->format = TextureFormat::RGBA32F;
		cubemapDesc->target = TextureTarget::Texture_Cube;
		cubemapDesc->size = vec2i(256);
		cubemapDesc->isRenderTarget = true;
		_envCubemap = renderer->createTexture(cubemapDesc);
	}

	if (_wrapCubemapPass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.camera = Camera::Pointer(PointerInit::CreateInplace);
		passInfo.color[0].enabled = true;
		passInfo.color[0].texture = _envCubemap;
		passInfo.color[0].loadOperation = FramebufferOperation::Clear;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = false;
		passInfo.name = "eq-to-cubemap";
		passInfo.priority = RenderPassPriority::Default + 0x100;
		_wrapCubemapPass = renderer->allocateRenderPass(passInfo);
		_wrapCubemapBatch = renderhelper::createFullscreenRenderBatch(_envTexture, _cubemapMaterial);
	}

	if (_cubemapDebugPass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.camera = Camera::Pointer(PointerInit::CreateInplace);
		passInfo.color[0].enabled = true;
		passInfo.color[0].loadOperation = FramebufferOperation::Load;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = true;
		passInfo.name = "cubemap-visualize";
		passInfo.priority = RenderPassPriority::UI - 1;
		_cubemapDebugPass = renderer->allocateRenderPass(passInfo);
		_cubemapDebugBatch = renderhelper::createFullscreenRenderBatch(_envCubemap, _cubemapMaterial);
		_cubemapDebugBatch->setTransformation(scaleMatrix(0.5f, 0.5f * Camera::renderingOriginTransform, 0.5f));
	}
}

}
}
