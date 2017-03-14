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
	for (uint32_t level = 0; level < CubemapLevels; ++level)
	{
		for (uint32_t layer = 0; layer < 6; ++layer)
			_env.wholeCubemapBeginInfo.subpasses.emplace_back(layer, level);
	}
	
	Camera cm;
	const mat4& proj = cm.perspectiveProjection(HALF_PI, 1.0f, 1.0f, 2.0f);
	_env.projections = cubemapMatrixProjectionArray(proj, vec3(0.0f));
	for (mat4& m : _env.projections)
		m = m.inverted();
}

void Renderer::render(RenderInterface::Pointer& renderer, const Scene::Pointer& scene)
{
	extractBatches(scene);

	if (_env.environmentMaterial.invalid())
	{
		_env.environmentMaterial = renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::EnvironmentMap);
	}

	processCubemap(renderer);
	validateMainPass(renderer, scene);
	validateShadowPass(renderer);

	if (_shadowPass.valid())
	{
		clip(_shadowPass, _renderBatches, _shadowPassBatches);
		render(_shadowPass, _shadowPassBatches);
		renderer->submitRenderPass(_shadowPass);
	}

	clip(_mainPass, _renderBatches, _mainPassBatches);

	if (_env.forwardBatch.valid())
		_mainPassBatches.emplace_back(_env.forwardBatch);

	render(_mainPass, _mainPassBatches);
	renderer->submitRenderPass(_mainPass);

	renderDebug(renderer);
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
	bool sameEnvLight = (environmentLight.valid() && _env.tex[CubemapType::Source].valid()) ? 
		(environmentLight->environmentMap() == _env.tex[CubemapType::Source]->origin()) : true;

	if (mainPassValid && sameCamera && sameDirectionalLight && sameEnvLight)
		return;

	if (environmentLight.valid() && (environmentLight->type() == Light::Type::ImageBasedSky))
	{
		_env.tex[CubemapType::Source] = renderer->loadTexture(environmentLight->environmentMap(), _cache);
		_env.forwardBatch = renderhelper::createFullscreenRenderBatch(_env.tex[CubemapType::Convoluted], _env.environmentMaterial);
	}
	else
	{
		_env.tex[CubemapType::Source] = renderer->blackTexture();
		_env.forwardBatch.reset(nullptr);
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
	_mainPass->setSharedTexture(MaterialTexture::Environment, _env.tex[CubemapType::Convoluted], renderer->defaultSampler());
	_mainPass->setSharedTexture(MaterialTexture::BRDFLookup, _env.lookup, renderer->clampSampler());

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
	pass->begin({ 0, 0 });
	for (const RenderBatchInfo& rb : batches)
		pass->pushRenderBatch(rb.batch);
	pass->end();
}

void Renderer::validateWrapCubemapPasses(RenderInterface::Pointer& renderer)
{
	if (_env.processingMaterial.invalid())
	{
		_env.processingMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap.json"));
	}

	if (_env.eqMapSampler.invalid())
	{
		Sampler::Description desc;
		desc.wrapU = TextureWrap::Repeat;
		desc.wrapV = TextureWrap::ClampToEdge;
		_env.eqMapSampler = renderer->createSampler(desc);
	}

	{
		TextureDescription::Pointer cubemapDesc(PointerInit::CreateInplace);
		cubemapDesc->format = TextureFormat::RGBA32F;
		cubemapDesc->target = TextureTarget::Texture_Cube;
		cubemapDesc->isRenderTarget = true;
		cubemapDesc->levelCount = CubemapLevels;
		cubemapDesc->size = vec2i(1 << (CubemapLevels - 1));
		if (_env.tex[CubemapType::Downsampled].invalid())
			_env.tex[CubemapType::Downsampled] = renderer->createTexture(cubemapDesc);
		if (_env.tex[CubemapType::Convoluted].invalid())
			_env.tex[CubemapType::Convoluted] = renderer->createTexture(cubemapDesc);
	}

	uint32_t passPriority = RenderPassPriority::Default + 0x100;

	if (_env.lookupPass.invalid())
	{
		TextureDescription::Pointer lookupDesc(PointerInit::CreateInplace);
		lookupDesc->format = TextureFormat::RGBA32F;
		lookupDesc->size = vec2i(256);
		_env.lookup = renderer->createTexture(lookupDesc);

		RenderPass::ConstructionInfo passInfo;
		passInfo.camera = Camera::Pointer(PointerInit::CreateInplace);

		passInfo.color[0].enabled = true;
		passInfo.color[0].texture = _env.lookup;
		passInfo.color[0].loadOperation = FramebufferOperation::DontCare;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = false;
		passInfo.name = "generate-split-sum-approx";
		passInfo.priority = passPriority--;
		_env.lookupPass = renderer->allocateRenderPass(passInfo);

		passInfo.color[0].loadOperation = FramebufferOperation::Load;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = true;
		passInfo.name = "forward";
		passInfo.priority = RenderPassPriority::UI - 2;
		_env.lookupDebugPass = renderer->allocateRenderPass(passInfo);

		_env.lookupDebugMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2dtransformed.json"));
		_env.lookupDebugBatch = renderhelper::createFullscreenRenderBatch(_env.lookup, _env.lookupDebugMaterial);
	}
	
	if (_env.downsamplePass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.camera = Camera::Pointer(PointerInit::CreateInplace);
		passInfo.color[0].enabled = true;
		passInfo.color[0].texture = _env.tex[CubemapType::Downsampled];
		passInfo.color[0].loadOperation = FramebufferOperation::DontCare;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = false;
		passInfo.name = "eq-to-cubemap";
		passInfo.priority = passPriority--;
		_env.downsamplePass = renderer->allocateRenderPass(passInfo);
		_env.downsampleBatch = renderhelper::createFullscreenRenderBatch(_env.tex[CubemapType::Source], _env.processingMaterial);
	}

	if (_env.specularConvolvePass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.camera = Camera::Pointer(PointerInit::CreateInplace);
		passInfo.color[0].enabled = true;
		passInfo.color[0].texture = _env.tex[CubemapType::Convoluted];
		passInfo.color[0].loadOperation = FramebufferOperation::DontCare;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = false;
		passInfo.name = "cubemap-specular-convolution";
		passInfo.priority = passPriority--;
		_env.specularConvolvePass = renderer->allocateRenderPass(passInfo);
		_env.specularConvolveBatch = renderhelper::createFullscreenRenderBatch(_env.tex[CubemapType::Downsampled], _env.processingMaterial);
	}

	if (_env.cubemapDebugPass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.camera = Camera::Pointer(PointerInit::CreateInplace);
		passInfo.color[0].enabled = true;
		passInfo.color[0].loadOperation = FramebufferOperation::Load;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = true;
		passInfo.name = "cubemap-visualize";
		passInfo.priority = RenderPassPriority::UI - 1;
		_env.cubemapDebugPass = renderer->allocateRenderPass(passInfo);
		_env.cubemapDebugBatch = renderhelper::createFullscreenRenderBatch(renderer->checkersTexture(), _env.processingMaterial);
	}
}

mat4 Renderer::fullscreenBatchTransform(const vec2& viewport, const vec2& origin, const vec2& size)
{
	vec2 fsz = size / viewport;
	vec2 fps = origin / viewport;
	mat4 result = scaleMatrix(fsz.x, fsz.y, 1.0f);
	result[3].x = 2.0f * (fps.x + 0.5f * fsz.x) - 1.0f;
	result[3].y = 2.0f * (fps.y + 0.5f * fsz.y) - 1.0f;
	return result;
}

void Renderer::renderDebug(RenderInterface::Pointer& renderer)
{
	vec2 vp = vector2ToFloat(renderer->rc()->size());
	
	if (options.drawEnvironmentProbe)
	{
		float dy = vp.y / static_cast<float>(CubemapLevels);
		float dx = 2.0f * dy;
		vec2 pos = vec2(0.0f, 0.0f);

		_env.cubemapDebugPass->begin({ 0, 0 });
		for (uint32_t i = CubemapType::Downsampled; i < CubemapType::Count; ++i)
		{
			pos.y = 0.0f;
			_env.cubemapDebugBatch->material()->setTexture(MaterialTexture::BaseColor, _env.tex[i]);
			for (uint32_t i = 0; i < CubemapLevels; ++i)
			{
				_env.cubemapDebugBatch->material()->setFloat(MaterialParameter::RoughnessScale, static_cast<float>(i));
				_env.cubemapDebugBatch->setTransformation(fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
				_env.cubemapDebugPass->pushRenderBatch(_env.cubemapDebugBatch);
				pos.y += dy;
			}
			pos.x += dx;
		}

		_env.cubemapDebugPass->end();
		renderer->submitRenderPass(_env.cubemapDebugPass);
	}

	if (options.drawLookupTexture)
	{
		vec2 lookupSize = vec2(256.0f);
		_env.lookupDebugBatch->setTransformation(fullscreenBatchTransform(vp, 0.5f * (vp - lookupSize), lookupSize));
		_env.lookupDebugPass->begin({ 0, 0 });
		_env.lookupDebugPass->pushRenderBatch(_env.lookupDebugBatch);
		_env.lookupDebugPass->end();
		renderer->submitRenderPass(_env.lookupDebugPass);
	}
}

void Renderer::processCubemap(RenderInterface::Pointer& renderer)
{
	validateWrapCubemapPasses(renderer);

	if (_state & RebuildLookupTexture)
	{
		_env.lookupPass->begin({ 0, 0 });
		_env.lookupPass->pushRenderBatch(renderhelper::createFullscreenRenderBatch(renderer->checkersTexture(), _env.environmentMaterial));
		_env.lookupPass->end();
		renderer->submitRenderPass(_env.lookupPass);
		_state &= ~RebuildLookupTexture;
	}

	if (_state & RebuildCubemap)
	{
		/*
		 * Downsampling convolution
		 */
		_env.downsampleBatch->material()->setTexture(MaterialTexture::BaseColor, _env.tex[CubemapType::Source]);
		_env.downsampleBatch->material()->setSampler(MaterialTexture::BaseColor, _env.eqMapSampler);
		_env.downsamplePass->begin(_env.wholeCubemapBeginInfo);
		for (uint32_t i = 0, e = static_cast<uint32_t>(_env.wholeCubemapBeginInfo.subpasses.size()); i < e; ++i)
		{
			_env.downsampleBatch->setTransformation(_env.projections[i % 6]);
			_env.downsamplePass->pushRenderBatch(_env.downsampleBatch);
			_env.downsamplePass->nextSubpass();
		}
		_env.downsamplePass->end();
		renderer->submitRenderPass(_env.downsamplePass);

		Material::Pointer mtl = _env.specularConvolveBatch->material();
		mtl->setTexture(MaterialTexture::BaseColor, _env.tex[CubemapType::Downsampled]);
		_env.specularConvolvePass->begin(_env.wholeCubemapBeginInfo);
		for (uint32_t i = 0, e = static_cast<uint32_t>(_env.wholeCubemapBeginInfo.subpasses.size()); i < e; ++i)
		{
			uint32_t level = i / 6;
			uint32_t face = i % 6;
			vec2 sz = vector2ToFloat(_env.tex[CubemapType::Downsampled]->size(level));
			mtl->setVector(MaterialParameter::RoughnessScale, vec4(static_cast<float>(level), sz.x, sz.y, static_cast<float>(face)));
			_env.specularConvolveBatch->setTransformation(_env.projections[face]);
			_env.specularConvolvePass->pushRenderBatch(_env.specularConvolveBatch);
			_env.specularConvolvePass->nextSubpass();
		}
		_env.specularConvolvePass->end();
		renderer->submitRenderPass(_env.specularConvolvePass);
		_state &= ~RebuildCubemap;
	}
}

}
}
