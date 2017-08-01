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

#define ET_ANIMATE_LIGHT_POSITION 0

namespace et
{
namespace s3d
{

static const vec2 sobolSequence[] = {
	vec2(0.000000, 0.000000),
	vec2(0.500000, 0.500000),
	vec2(0.750000, 0.250000),
	vec2(0.250000, 0.750000),
	vec2(0.375000, 0.375000),
	vec2(0.875000, 0.875000),
	vec2(0.625000, 0.125000),
	vec2(0.125000, 0.625000),
	vec2(0.187500, 0.312500),
	vec2(0.687500, 0.812500),
};
static const uint64_t sobolSequenceSize = sizeof(sobolSequence) / sizeof(sobolSequence[0]);

Drawer::Drawer(const RenderInterface::Pointer& renderer) :
	_renderer(renderer)
{
	_debugDrawer = DebugDrawer::Pointer::create(renderer);
	_main.noise = _renderer->loadTexture(application().resolveFileName("engine_data/textures/bluenoise.png"), _cache);

	Sampler::Description shadowSamplerDesc;
	shadowSamplerDesc.wrapU = TextureWrap::ClampToEdge;
	shadowSamplerDesc.wrapV = TextureWrap::ClampToEdge;
	shadowSamplerDesc.wrapW = TextureWrap::ClampToEdge;
	shadowSamplerDesc.minFilter = TextureFiltration::Linear;
	shadowSamplerDesc.magFilter = TextureFiltration::Linear;
	shadowSamplerDesc.mipFilter = TextureFiltration::Nearest;
	shadowSamplerDesc.compareEnabled = true;
	shadowSamplerDesc.compareFunction = CompareFunction::LessOrEqual;
	_main.shadowSampler = _renderer->createSampler(shadowSamplerDesc);

	Scene::Pointer scene(PointerInit::CreateInplace);
	scene->setRenderCamera(Camera::Pointer(PointerInit::CreateInplace));
	scene->setClipCamera(scene->renderCamera());
	setScene(scene);
}

void Drawer::draw()
{
#if (ET_ANIMATE_LIGHT_POSITION)
	_lighting.directional->lookAt(10.0f * fromSpherical(0.25f * queryContiniousTimeInSeconds(), DEG_15));
	options.rebuldEnvironmentProbe = true;
#endif

	_debugDrawer->begin();
	_cubemapProcessor->process(_renderer, options, _lighting.directional);
	_shadowmapProcessor->process(_renderer, options);

	validate(_renderer);

	vec2 ji = sobolSequence[_frameIndex % sobolSequenceSize];
	vec2 jj = sobolSequence[(_frameIndex + sobolSequenceSize - 1) % sobolSequenceSize];
	_jitter.x = (ji.x * 2.0f - 1.0f) / static_cast<float>(_main.color->size(0).x);
	_jitter.y = (ji.y * 2.0f - 1.0f) / static_cast<float>(_main.color->size(0).y);
	_jitter.z = (jj.x * 2.0f - 1.0f) / static_cast<float>(_main.color->size(0).x);
	_jitter.w = (jj.y * 2.0f - 1.0f) / static_cast<float>(_main.color->size(0).y);

	Camera::Pointer renderCamera = _scene->renderCamera();
	Camera::Pointer clipCamera = _scene->clipCamera();
	renderCamera->setProjectionMatrix(_baseProjectionMatrix * translationMatrix(_jitter.x, _jitter.y, 0.0f));
	{
		_main.pass->loadSharedVariablesFromCamera(renderCamera);
		_main.pass->loadSharedVariablesFromLight(_lighting.directional);

		_main.pass->begin(RenderPassBeginInfo::singlePass());
		_main.pass->nextSubpass();
		for (Mesh::Pointer mesh : _allMeshes)
		{
			const mat4& transform = mesh->transform();
			const mat4& rotationTransform = mesh->rotationTransform();
			_main.pass->setSharedVariable(ObjectVariable::WorldTransform, transform);
			_main.pass->setSharedVariable(ObjectVariable::WorldRotationTransform, rotationTransform);
			for (const RenderBatch::Pointer& rb : mesh->renderBatches())
				_main.pass->pushRenderBatch(rb);
		}
		_main.pass->setSharedVariable(ObjectVariable::WorldTransform, identityMatrix);
		_main.pass->pushRenderBatch(_lighting.environmentBatch);

		_debugDrawer->drawCameraFrustum(_lighting.directional, vec4(10000.0f, 20000.0f, 0.0f, 1.0f));
		_debugDrawer->drawBoundingBox(_shadowmapProcessor->sceneBoundingBox(), identityMatrix, vec4(10000.0f, 10000.0f, 10000.0f, 1.0f));
		_debugDrawer->submitBatches(_main.pass);

		_main.pass->endSubpass();
		_main.pass->end();

		_renderer->submitRenderPass(_main.pass);
	}
	++_frameIndex;
}

void Drawer::validate(RenderInterface::Pointer& renderer)
{
	ET_ASSERT(_main.color.valid());

	if (_main.pass.invalid() || (_main.pass->info().color[0].texture != _main.color))
	{
		TextureDescription::Pointer desc(PointerInit::CreateInplace);
		desc->size = _main.color->size(0);
		desc->format = TextureFormat::RG32F;
		desc->flags |= Texture::Flags::RenderTarget;
		_main.velocity = renderer->createTexture(desc);

		desc->format = TextureFormat::Depth32F;
		_main.depth = renderer->createTexture(desc);

		RenderPass::ConstructionInfo passInfo;
		passInfo.name = RenderPass::kPassNameDefault;

		passInfo.color[0].texture = _main.color;
		passInfo.color[0].loadOperation = FramebufferOperation::Clear;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].enabled = true;
		passInfo.color[0].clearValue = vec4(0.0f, 1.0f);
		passInfo.color[0].useDefaultRenderTarget = false;

		passInfo.color[1].texture = _main.velocity;
		passInfo.color[1].loadOperation = FramebufferOperation::Clear;
		passInfo.color[1].storeOperation = FramebufferOperation::Store;
		passInfo.color[1].enabled = true;
		passInfo.color[1].clearValue = vec4(0.0f, 1.0f);
		passInfo.color[1].useDefaultRenderTarget = false;

		passInfo.depth.texture = _main.depth;
		passInfo.depth.loadOperation = FramebufferOperation::Clear;
		passInfo.depth.storeOperation = FramebufferOperation::DontCare;
		passInfo.depth.enabled = true;
		passInfo.depth.useDefaultRenderTarget = false;

		_main.pass = renderer->allocateRenderPass(passInfo);
		_main.pass->setSharedTexture(MaterialTexture::Environment, _cubemapProcessor->convolutedCubemap(), renderer->defaultSampler());
		_main.pass->setSharedTexture(MaterialTexture::Shadow, _shadowmapProcessor->directionalShadowmap(), _main.shadowSampler);
		_main.pass->setSharedTexture(MaterialTexture::BRDFLookup, _cubemapProcessor->brdfLookupTexture(), renderer->clampSampler());
		_main.pass->setSharedTexture(MaterialTexture::Noise, _main.noise, renderer->nearestSampler());
	}

	if (_lighting.environmentMaterial.invalid())
		_lighting.environmentMaterial = renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::EnvironmentMap);

	if (_lighting.environmentBatch.invalid())
		_lighting.environmentBatch = renderhelper::createFullscreenRenderBatch(_cubemapProcessor->convolutedCubemap(), _lighting.environmentMaterial);

	_cache.flush();
}

void Drawer::setRenderTarget(const Texture::Pointer& tex)
{
	_main.color = tex;
}

void Drawer::setScene(const Scene::Pointer& inScene)
{
	_scene = inScene;
	BaseElement::List elements = _scene->childrenOfType(ElementType::DontCare);

	_allMeshes.clear();
	_allMeshes.reserve(elements.size());
	_lighting.directional.reset(nullptr);

	bool updateEnvironment = false;

	for (BaseElement::Pointer element : elements)
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
		vec3 lightPoint = 10.0f * fromSpherical(DEG_60, DEG_15);
		_lighting.directional = Light::Pointer::create(Light::Type::Directional);
		_lighting.directional->setColor(vec3(120000.0f));
		_lighting.directional->lookAt(lightPoint);
		_lighting.directional->perspectiveProjection(QUARTER_PI, 1.0f, 1.0f, 1000.0f);
	}

	_shadowmapProcessor->setScene(_scene, _lighting.directional);
}

void Drawer::setEnvironmentMap(const std::string& filename)
{
	Texture::Pointer tex = _renderer->loadTexture(filename, _cache);
	_cubemapProcessor->processEquiretangularTexture(tex.valid() ? tex : _renderer->checkersTexture());
}

void Drawer::updateBaseProjectionMatrix(const mat4& m)
{
	_baseProjectionMatrix = m;
}

void Drawer::updateLight()
{
	options.rebuldEnvironmentProbe = true;
	_shadowmapProcessor->updateLight(_lighting.directional);
}

}
}
