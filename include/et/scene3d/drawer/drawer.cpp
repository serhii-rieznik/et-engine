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
#include <et/imaging/pngloader.h>
#include <et/imaging/imagewriter.h>
#include <et/app/application.h>

#define ET_ANIMATE_LIGHT_POSITION 0

namespace et {
namespace s3d {

static const vec2 sobolSequence[] = {
	vec2(-1.0f, -1.0f),
	vec2(0.0f, 0.0f),
	vec2(-0.5f, 0.5f),
	vec2(0.5f, -0.5f),
	vec2(-0.75f, 0.25f),
	vec2(0.25f, -0.75f),
	vec2(-0.25f, -0.25f),
	vec2(0.75f, 0.75f),
	vec2(-0.875f, 0.875f),
	vec2(0.125f, -0.125f),
	vec2(-0.375f, -0.625f),
	vec2(0.625f, 0.375f),
	vec2(-0.625f, -0.375f),
	vec2(0.375f, 0.625f),
	vec2(-0.125f, 0.125f),
	vec2(0.875f, -0.875f),
	vec2(-0.9375f, 0.0625f),
	vec2(0.0625f, -0.9375f),
	vec2(-0.4375f, -0.4375f),
	vec2(0.5625f, 0.5625f),
	vec2(-0.6875f, -0.6875f),
	vec2(0.3125f, 0.3125f),
	vec2(-0.1875f, 0.8125f),
	vec2(0.8125f, -0.1875f),
	vec2(-0.8125f, -0.0625f),
	vec2(0.1875f, 0.9375f),
	vec2(-0.3125f, 0.4375f),
	vec2(0.6875f, -0.5625f),
	vec2(-0.5625f, 0.6875f),
	vec2(0.4375f, -0.3125f),
	vec2(-0.0625f, -0.8125f),
	vec2(0.9375f, 0.1875f),
};
static const uint64_t sobolSequenceSize = sizeof(sobolSequence) / sizeof(sobolSequence[0]);

Drawer::Drawer(RenderInterface::Pointer& renderer) :
	_renderer(renderer) {
	_debugDrawer = DebugDrawer::Pointer::create(renderer);
	_main.noise = _renderer->loadTexture(application().resolveFileName("engine_data/textures/bluenoise.sincos.png"), _cache);
	_main.ltcTransform = _renderer->loadTexture(application().resolveFileName("engine_data/textures/ltc_mat.dds"), _cache);
	/*
	float minV = 1000.0f;
	float maxV = -minV;
	TextureDescription desc;
	png::loadFromFile(application().resolveFileName("engine_data/textures/bluenoise.png"), desc, false);
	vec4ub* ptr = reinterpret_cast<vec4ub*>(desc.data.binary());
	for (uint32_t i = 0; i < 64 * 64; ++i)
	{
		vec4ub value = *ptr;
		float t = static_cast<float>(value.z) / 255.0f * DOUBLE_PI;
		float s = std::sin(t);
		float c = std::cos(t);
		value.z = static_cast<uint8_t>(clamp((s * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
		value.w = static_cast<uint8_t>(clamp((c * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));

		float rz = (static_cast<float>(value.z) / 255.0f) * 2.0 - 1.0f;
		float rw = (static_cast<float>(value.w) / 255.0f) * 2.0 - 1.0f;
		float r = rz*rz + rw*rw;
		minV = std::min(minV, r);
		maxV = std::max(maxV, r);

		*ptr++ = value;
	}
	auto dst = application().environment().applicationDocumentsFolder() + "bluenoise.sincos.png";
	writeImageToFile(dst, desc.data, vec2i(64, 64), 4, 8, ImageFormat::ImageFormat_PNG, false);
	log::warning("Processed image saved to: %s", dst.c_str());
	// */
	Scene::Pointer scene(PointerInit::CreateInplace);
	scene->setRenderCamera(Camera::Pointer(PointerInit::CreateInplace));
	scene->setClipCamera(scene->renderCamera());
	setScene(scene);
}

void Drawer::updateVisibleMeshes() {
	_visibleMeshes.clear();
	_visibleMeshes.reserve(_allMeshes.size());
	for (Mesh::Pointer& mesh : _allMeshes)
	{
		if (_scene->renderCamera()->frustum().containsBoundingBox(mesh->tranformedBoundingBox()))
		{
			_visibleMeshes.emplace_back(mesh);
		}
	}
}

void Drawer::draw() {
#if (ET_ANIMATE_LIGHT_POSITION)
	_lighting.directional->lookAt(10.0f * fromSpherical(0.25f * queryContinuousTimeInSeconds(), DEG_15));
	options.rebuldEnvironmentProbe = true;
#endif

	_cubemapProcessor->process(_renderer, options, _lighting.directional);
	_shadowmapProcessor->process(_renderer, options);

	validate(_renderer);

	vec2 jitterScale = vec2(1.0f) / vector2ToFloat(_main.color->size(0));
	vec2 ji = jitterScale * sobolSequence[_frameIndex % sobolSequenceSize];

	_jitter.z = _jitter.x;
	_jitter.w = _jitter.y;
	_jitter.x = ji.x;
	_jitter.y = ji.y;

	mat4 projectionMatrix = _scene->renderCamera()->projectionMatrix();
	projectionMatrix[2].x = _jitter.x;
	projectionMatrix[2].y = _jitter.y;
	_scene->renderCamera()->setProjectionMatrix(projectionMatrix);

	updateVisibleMeshes();

	_renderer->beginRenderPass(_main.zPrepass, RenderPassBeginInfo::singlePass());
	{
		_main.zPrepass->setSharedVariable(ObjectVariable::CameraJitter, _jitter);
		_main.zPrepass->loadSharedVariablesFromCamera(_scene->renderCamera());
		_main.zPrepass->nextSubpass();
		for (Mesh::Pointer& mesh : _visibleMeshes)
		{
			_main.zPrepass->setSharedVariable(ObjectVariable::WorldTransform, mesh->transform());
			for (const RenderBatch::Pointer& rb : mesh->renderBatches())
				_main.zPrepass->pushRenderBatch(rb);
		}
		_main.zPrepass->endSubpass();
	}
	_renderer->submitRenderPass(_main.zPrepass);

	if (options.enableScreenSpaceShadows)
	{
		_main.screenSpaceShadows->setSharedVariable(ObjectVariable::CameraJitter, _jitter);
		_main.screenSpaceShadows->loadSharedVariablesFromCamera(_scene->renderCamera());
		_main.screenSpaceShadows->loadSharedVariablesFromLight(_lighting.directional);
		_renderer->submitPassWithRenderBatch(_main.screenSpaceShadows, _main.screenSpaceShadowsBatch);
	}

	if (options.enableScreenSpaceAO)
	{
		_main.screenSpaceAO->setSharedVariable(ObjectVariable::CameraJitter, _jitter);
		_main.screenSpaceAO->setSharedTexture(MaterialTexture::Noise, _main.noise, _renderer->nearestSampler());
		_main.screenSpaceAO->loadSharedVariablesFromCamera(_scene->renderCamera());
		_main.screenSpaceAO->loadSharedVariablesFromLight(_lighting.directional);
		_renderer->submitPassWithRenderBatch(_main.screenSpaceAO, _main.screenSpaceAOBatch);
	}

	_renderer->beginRenderPass(_main.forward, RenderPassBeginInfo::singlePass());
	{
		_main.forward->loadSharedVariablesFromCamera(_scene->renderCamera());
		_main.forward->loadSharedVariablesFromLight(_lighting.directional);
		_main.forward->setSharedTexture(MaterialTexture::Shadow, _shadowmapProcessor->directionalShadowmap(), _shadowmapProcessor->directionalShadowmapSampler());
		_main.forward->setSharedTexture(MaterialTexture::AmbientOcclusion, _main.screenSpaceAOTexture, _renderer->defaultSampler());
		_main.forward->setSharedVariable(ObjectVariable::EnvironmentSphericalHarmonics, _cubemapProcessor->environmentSphericalHarmonics(), 9);
		_main.forward->setSharedVariable(ObjectVariable::CameraJitter, _jitter);
		_main.forward->nextSubpass();
		for (Mesh::Pointer& mesh : _visibleMeshes)
		{
			_main.forward->setSharedVariable(ObjectVariable::WorldTransform, mesh->transform());
			_main.forward->setSharedVariable(ObjectVariable::WorldRotationTransform, mesh->rotationTransform());
			_main.forward->setSharedVariable(ObjectVariable::PreviousWorldTransform, _previousFrameTransforms[mesh].first);
			_main.forward->setSharedVariable(ObjectVariable::PreviousWorldRotationTransform, _previousFrameTransforms[mesh].second);

			for (const RenderBatch::Pointer& rb : mesh->renderBatches())
				_main.forward->pushRenderBatch(rb);

			_previousFrameTransforms[mesh] = std::make_pair(mesh->transform(), mesh->rotationTransform());
		}
		_main.forward->setSharedVariable(ObjectVariable::WorldTransform, identityMatrix);
		_main.forward->pushRenderBatch(_lighting.environmentBatch);
		_main.forward->endSubpass();
	}
	_renderer->submitRenderPass(_main.forward);
	++_frameIndex;
}

void Drawer::validate(RenderInterface::Pointer& renderer) {
	ET_ASSERT(_main.color.valid());

	if (_main.forward.invalid() || (_main.forward->info().color[0].texture != _main.color))
	{
		TextureDescription::Pointer desc(PointerInit::CreateInplace);
		desc->size = _main.color->size(0);
		desc->format = TextureFormat::RG32F;
		desc->flags |= Texture::Flags::RenderTarget;
		_main.velocity = renderer->createTexture(desc);

		desc->format = TextureFormat::Depth32F;
		_main.depth = renderer->createTexture(desc);

		desc->format = TextureFormat::RGBA8;
		_main.screenSpaceShadowsTexture = renderer->createTexture(desc);
		_main.screenSpaceAOTexture = renderer->createTexture(desc);

		{
			Material::Pointer material = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/screen-space-shadows.json"));
			_main.screenSpaceShadowsBatch = renderhelper::createQuadBatch("sceneDepth", _main.depth, material);

			RenderPass::ConstructionInfo screenSpaceShadowsInfo("screen-space-shadows");
			screenSpaceShadowsInfo.color[0].texture = _main.screenSpaceShadowsTexture;
			screenSpaceShadowsInfo.color[0].targetClass = RenderTarget::Class::Texture;
			_main.screenSpaceShadows = renderer->allocateRenderPass(screenSpaceShadowsInfo);
		}

		{
			Material::Pointer material = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/screen-space-ao.json"));
			_main.screenSpaceAOBatch = renderhelper::createQuadBatch("sceneDepth", _main.depth, material);

			RenderPass::ConstructionInfo screenSpaceAOInfo("screen-space-ao");
			screenSpaceAOInfo.color[0].texture = _main.screenSpaceAOTexture;
			screenSpaceAOInfo.color[0].targetClass = RenderTarget::Class::Texture;
			_main.screenSpaceAO = renderer->allocateRenderPass(screenSpaceAOInfo);
		}

		RenderPass::ConstructionInfo passInfo;
		passInfo.name = "forward";

		passInfo.color[0].texture = _main.color;
		passInfo.color[0].loadOperation = FramebufferOperation::Clear;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::Texture;
		passInfo.color[0].clearValue = vec4(0.0f, 1.0f);

		passInfo.color[1].texture = _main.velocity;
		passInfo.color[1].loadOperation = FramebufferOperation::Clear;
		passInfo.color[1].storeOperation = FramebufferOperation::Store;
		passInfo.color[1].targetClass = RenderTarget::Class::Texture;
		passInfo.color[1].clearValue = vec4(0.0f, 1.0f);

		passInfo.depth.texture = _main.depth;
		passInfo.depth.loadOperation = FramebufferOperation::Load;
		passInfo.depth.storeOperation = FramebufferOperation::DontCare;
		passInfo.depth.targetClass = RenderTarget::Class::Texture;

		_main.forward = renderer->allocateRenderPass(passInfo);
		_main.forward->setSharedTexture(MaterialTexture::ConvolvedSpecular, _cubemapProcessor->convolvedSpecularCubemap());
		_main.forward->setSharedTexture(MaterialTexture::BRDFLookup, _cubemapProcessor->brdfLookupTexture());
		_main.forward->setSharedTexture(MaterialTexture::Noise, _main.noise);
		_main.forward->setSharedTexture(MaterialTexture::LTCTransform, _main.ltcTransform);

		passInfo.name = "z-prepass";
		passInfo.color[0].targetClass = RenderTarget::Class::Disabled;
		passInfo.color[1].targetClass = RenderTarget::Class::Disabled;
		passInfo.depth.loadOperation = FramebufferOperation::Clear;
		passInfo.depth.storeOperation = FramebufferOperation::Store;
		_main.zPrepass = renderer->allocateRenderPass(passInfo);
	}

	if (_lighting.environmentMaterial.invalid())
		_lighting.environmentMaterial = renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::EnvironmentMap);

	if (_lighting.environmentBatch.invalid())
		_lighting.environmentBatch = renderhelper::createQuadBatch("convolvedSpecular", _cubemapProcessor->convolvedSpecularCubemap(), _lighting.environmentMaterial);

	_cache.flush();
}

void Drawer::setRenderTarget(const Texture::Pointer& tex) {
	_main.color = tex;
}

void Drawer::setScene(const Scene::Pointer& inScene) {
	_scene = inScene;
	BaseElement::List elements = _scene->childrenOfType(ElementType::DontCare);

	_allMeshes.clear();
	_allMeshes.reserve(elements.size());
	_lighting.directional.reset(nullptr);

	bool updateEnvironment = false;

	for (const BaseElement::Pointer& element : elements)
	{
		if (element->type() == ElementType::Mesh)
		{
			_allMeshes.emplace_back(element);
		}
		else if (element->type() == ElementType::Light)
		{
			Light::Pointer light = LightElement::Pointer(element)->light();
			switch (light->type())
			{
			case Light::Type::Directional:
			{
				_lighting.directional = light;
				break;
			}
			case Light::Type::ImageBasedEnvironment:
			{
				updateEnvironment = (_lighting.environmentTextureFile != light->environmentMap());
				_lighting.environmentTextureFile = light->environmentMap();
				break;
			}
			case Light::Type::UniformColorEnvironment:
			{
				_lighting.environmentTextureFile.clear();
				break;
			}
			default:
				ET_FAIL_FMT("Unsupported light type: %u", static_cast<uint32_t>(light->type()));
			}
		}
	}

	if (updateEnvironment)
	{
		setEnvironmentMap(_lighting.environmentTextureFile);
	}

	if (_lighting.directional.invalid())
	{
		vec3 lightDirection = fromSpherical(DEG_45, DEG_45);
		_lighting.directional = Light::Pointer::create(Light::Type::Directional);
		_lighting.directional->setColor(vec3(120000.0f));
		_lighting.directional->lookAt(lightDirection);
		_lighting.directional->perspectiveProjection(QUARTER_PI, 1.0f, 1.0f, 2048.0f);
	}

	_shadowmapProcessor->setScene(_scene, _lighting.directional);
}

void Drawer::setEnvironmentMap(const std::string& filename) {
	if (filename == "built-in:atmosphere")
	{
		_cubemapProcessor->processAtmosphere();
	}
	else
	{
		Texture::Pointer tex = _renderer->loadTexture(filename, _cache);
		_cubemapProcessor->processEquiretangularTexture(tex.valid() ? tex : _renderer->checkersTexture());
	}
}

void Drawer::updateLight() {
	options.rebuldEnvironmentProbe = true;
	_shadowmapProcessor->updateLight(_lighting.directional);
}

}
}
