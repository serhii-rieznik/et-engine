/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/drawer/drawer.h>
#include <et/rendering/rendercontext.h>
#include <et/rendering/base/primitives.h>
#include <et/rendering/base/helpers.h>
#include <et/app/application.h>

namespace et
{
namespace s3d
{

Drawer::Drawer() :
	FlagsHolder(RebuildLookupTexture)
{
	_cubemapProcessor = CubemapProcessor::Pointer(PointerInit::CreateInplace);
}

void Drawer::render(RenderInterface::Pointer& renderer, const Scene::Pointer& scene)
{
	extractBatches(scene);

	_cubemapProcessor->process(renderer, options);
	validateMainPass(renderer, scene);
	validateShadowPass(renderer);

	if (_shadowPass.valid())
	{
		clip(_shadowPass, _renderBatches, _shadowPassBatches);
		render(_shadowPass, _shadowPassBatches);
		renderer->submitRenderPass(_shadowPass);
	}

	clip(_mainPass, _renderBatches, _mainPassBatches);
	_mainPassBatches.emplace_back(_environmentMapBatch);

	render(_mainPass, _mainPassBatches);
	renderer->submitRenderPass(_mainPass);

	renderDebug(renderer);
}

void Drawer::validateMainPass(RenderInterface::Pointer& renderer, const Scene::Pointer& scene)
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
	bool sameEnvLight = environmentLight.invalid() || (_cubemapProcessor->sourceTextureName() == environmentLight->environmentMap());

	if (mainPassValid && sameCamera && sameDirectionalLight && sameEnvLight)
		return;

	if (environmentLight.valid() && (environmentLight->type() == Light::Type::ImageBasedSky))
	{
		Texture::Pointer env = renderer->loadTexture(environmentLight->environmentMap(), _cache);
		_cubemapProcessor->processEquiretangularTexture(env);
	}
	else
	{
		_cubemapProcessor->processEquiretangularTexture(renderer->checkersTexture());
	}

	if (_environmentMapBatch.invalid())
	{
		_environmentMaterial = renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::EnvironmentMap);
		_environmentMapBatch = renderhelper::createFullscreenRenderBatch(_cubemapProcessor->convolutedCubemap(), _environmentMaterial);
	}

	setFlag(RebuildCubemap);

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
	_mainPass->setSharedTexture(MaterialTexture::Environment, _cubemapProcessor->convolutedCubemap(), renderer->defaultSampler());
	_mainPass->setSharedTexture(MaterialTexture::BRDFLookup, _cubemapProcessor->brdfLookupTexture(), renderer->clampSampler());

	_shadowTexture.reset(nullptr);
	_cache.flush();
}

void Drawer::validateShadowPass(RenderInterface::Pointer& renderer)
{
	if (_shadowTexture.invalid())
	{
		TextureDescription::Pointer desc = TextureDescription::Pointer::create();
		desc->isRenderTarget = true;
		desc->size = vec2i(2048); // TODO : variable size
		desc->format = TextureFormat::Depth32F;
		_shadowTexture = renderer->createTexture(desc);

		Sampler::Description smpDesc;
		smpDesc.minFilter = TextureFiltration::Nearest;
		smpDesc.magFilter = TextureFiltration::Nearest;
		smpDesc.wrapU = TextureWrap::ClampToEdge;
		smpDesc.wrapV = TextureWrap::ClampToEdge;
		_mainPass->setSharedTexture(MaterialTexture::Shadow, _shadowTexture, renderer->createSampler(smpDesc));
	}

	if (_mainPass->info().light.valid() && (_shadowPass.invalid() || (_shadowPass->info().camera != _mainPass->info().light)))
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.camera = _mainPass->info().light;
		passInfo.light = _mainPass->info().light;
		passInfo.priority = RenderPassPriority::Default + 1;
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

void Drawer::extractBatches(const Scene::Pointer& scene)
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

void Drawer::clip(RenderPass::Pointer& pass, const RenderBatchCollection& inBatches, RenderBatchInfoCollection& passBatches)
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

void Drawer::render(RenderPass::Pointer& pass, const RenderBatchInfoCollection& batches)
{
	pass->begin({ 0, 0 });
	for (const RenderBatchInfo& rb : batches)
		pass->pushRenderBatch(rb.batch);
	pass->end();
}

void Drawer::renderDebug(RenderInterface::Pointer& renderer)
{
}

}
}
