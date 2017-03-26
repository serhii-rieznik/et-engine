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

Drawer::Drawer(const RenderInterface::Pointer& renderer) : 
	_renderer(renderer)
{
	_cubemapProcessor = CubemapProcessor::Pointer(PointerInit::CreateInplace);
}

void Drawer::draw()
{
	_cubemapProcessor->process(_renderer, options, _lighting.directional);
	validate(_renderer);

	Camera::Pointer clipCamera = _scene.valid() ? _scene->clipCamera() : _defaultCamera;
	Camera::Pointer renderCamera = _scene.valid() ? _scene->renderCamera() : _defaultCamera;
	
	clip(clipCamera, _main.all, _main.rendereable);
	{
		_main.pass->loadSharedVariablesFromCamera(renderCamera);
		_main.pass->loadSharedVariablesFromLight(_lighting.directional);

		_main.pass->begin({ 0, 0 });
		for (const RenderBatch::Pointer& rb : _main.rendereable)
			_main.pass->pushRenderBatch(rb);
		_main.pass->pushRenderBatch(_lighting.environmentBatch);
		_main.pass->end();

		_renderer->submitRenderPass(_main.pass);
	}

	renderDebug(_renderer);
}

void Drawer::validate(RenderInterface::Pointer& renderer)
{
	if (_main.pass.invalid() || (_main.pass->info().color[0].texture != _main.renderTarget))
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].texture = _main.renderTarget;
		passInfo.color[0].loadOperation = FramebufferOperation::Clear;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].enabled = true;
		passInfo.color[0].clearValue = vec4(0.0f, 1.0f);
		passInfo.color[0].useDefaultRenderTarget = _main.renderTarget.invalid();
		passInfo.depth.loadOperation = FramebufferOperation::Clear;
		passInfo.depth.storeOperation = FramebufferOperation::DontCare;
		passInfo.depth.enabled = true;
		passInfo.name = RenderPass::kPassNameDefault;

		_main.pass = renderer->allocateRenderPass(passInfo);
		_main.pass->setSharedTexture(MaterialTexture::Environment, _cubemapProcessor->convolutedCubemap(), renderer->defaultSampler());
		_main.pass->setSharedTexture(MaterialTexture::BRDFLookup, _cubemapProcessor->brdfLookupTexture(), renderer->clampSampler());
	}

	if (_lighting.environmentMaterial.invalid())
		_lighting.environmentMaterial = renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::EnvironmentMap);

	if (_lighting.environmentBatch.invalid())
		_lighting.environmentBatch = renderhelper::createFullscreenRenderBatch(_cubemapProcessor->convolutedCubemap(), _lighting.environmentMaterial);

	_cache.flush();
}

void Drawer::setRenderTarget(const Texture::Pointer& tex)
{
	_main.renderTarget = tex;
}

void Drawer::setScene(const Scene::Pointer& inScene)
{
	_scene = inScene;
	BaseElement::List elements = _scene->childrenOfType(ElementType::DontCare);

	_main.all.clear();
	_main.all.reserve(2 * elements.size());

	bool updateEnvironment = false;

	for (BaseElement::Pointer element : elements)
	{
		if (element->type() == ElementType::Mesh)
		{
			Mesh::Pointer mesh = element;
			mesh->prepareRenderBatches();
			_main.all.insert(_main.all.end(), mesh->renderBatches().begin(), mesh->renderBatches().end());
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
			case Light::Type::ImageBasedSky:
			{
				updateEnvironment = (_lighting.environmentTextureFile != light->environmentMap());
				_lighting.environmentTextureFile = light->environmentMap();
				break;
			}
			case Light::Type::UniformColorSky:
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
}

void Drawer::setEnvironmentMap(const std::string& filename)
{
	Texture::Pointer tex = _renderer->loadTexture(filename, _cache);
	_cubemapProcessor->processEquiretangularTexture(tex.valid() ? tex : _renderer->checkersTexture());
}

void Drawer::clip(const Camera::Pointer& cam, const RenderBatchCollection& inBatches, RenderBatchCollection& passBatches)
{
	passBatches.clear();
	passBatches.reserve(inBatches.size());

	for (const RenderBatch::Pointer& batch : inBatches)
	{
		// TODO : frustum clipping
		passBatches.emplace_back(batch);
	}
	
	vec3 cameraPosition = cam->position();
	auto cmp = [cameraPosition, this](RenderBatch::Pointer l, RenderBatch::Pointer r)
	{
		const BlendState& lbs = l->material()->configuration(_main.pass->info().name).blendState;
		const BlendState& rbs = r->material()->configuration(_main.pass->info().name).blendState;
		if (lbs.enabled == rbs.enabled)
		{
			float delta = (l->transformedBoundingBox().center - cameraPosition).dotSelf() - 
				(r->transformedBoundingBox().center - cameraPosition).dotSelf();
			return lbs.enabled ? (delta >= 0.0f) : (delta < 0.0f);
		}
		return static_cast<int>(lbs.enabled) < static_cast<int>(rbs.enabled);
	};

	std::stable_sort(passBatches.begin(), passBatches.end(), cmp);
}

void Drawer::renderDebug(RenderInterface::Pointer& renderer)
{
}

}
}
