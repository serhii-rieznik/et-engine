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

Drawer::Drawer()
{
	_cubemapProcessor = CubemapProcessor::Pointer(PointerInit::CreateInplace);
}

void Drawer::draw(RenderInterface::Pointer& renderer)
{
	_cubemapProcessor->process(renderer, options);
	validate(renderer);

	clip(_main.camera, _main.rendereables, _main.batches);
	{
		_main.pass->loadSharedVariablesFromCamera(_main.camera);
		_main.pass->begin({ 0, 0 });
		for (const RenderBatchInfo& rb : _main.batches)
			_main.pass->pushRenderBatch(rb.batch);
		_main.pass->pushRenderBatch(_lighting.environmentBatch);
		_main.pass->end();

		renderer->submitRenderPass(_main.pass);
	}

	renderDebug(renderer);
}

void Drawer::validate(RenderInterface::Pointer& renderer)
{
	if (_main.pass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].loadOperation = FramebufferOperation::Clear;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].enabled = true;
		passInfo.color[0].clearValue = vec4(0.0f, 1.0f);
		passInfo.depth.loadOperation = FramebufferOperation::Clear;
		passInfo.depth.storeOperation = FramebufferOperation::DontCare;
		passInfo.depth.enabled = true;
		passInfo.name = RenderPass::kPassNameForward;
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

void Drawer::setScene(const Scene::Pointer& scene, RenderInterface::Pointer& renderer)
{
	BaseElement::List elements = scene->childrenOfType(ElementType::DontCare);

	_main.rendereables.clear();
	_main.rendereables.reserve(2 * elements.size());

	_main.camera = scene->mainCamera();
	ET_ASSERT(_main.camera.valid());

	bool updateEnvironment = false;

	for (BaseElement::Pointer element : elements)
	{
		if (element->type() == ElementType::Mesh)
		{
			Mesh::Pointer mesh = element;
			mesh->prepareRenderBatches();
			_main.rendereables.insert(_main.rendereables.end(), mesh->renderBatches().begin(), mesh->renderBatches().end());
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
				_lighting.environment = light;
				break;
			}
			case Light::Type::UniformColorSky:
			{
				_lighting.environmentTextureFile.clear();
				_lighting.environment = light;
				break;
			}
			default:
				ET_FAIL_FMT("Unsupported light type: %u", static_cast<uint32_t>(light->type()));
			}
		}
	}

	if (updateEnvironment)
	{
		Texture::Pointer env = renderer->loadTexture(_lighting.environmentTextureFile, _cache);
		_cubemapProcessor->processEquiretangularTexture(env.valid() ? env : renderer->checkersTexture());
	}
}

void Drawer::clip(const Camera::Pointer& cam, const RenderBatchCollection& inBatches, RenderBatchInfoCollection& passBatches)
{
	passBatches.clear();
	passBatches.reserve(inBatches.size());

	for (const RenderBatch::Pointer& batch : inBatches)
	{
		BoundingBox transformedBox = batch->boundingBox().transform(batch->transformation());
		if (1 || cam->frustum().containsBoundingBox(transformedBox))
		{
			uint64_t key = batch->material()->sortingKey();
			passBatches.emplace_back(key, batch, transformedBox);
		}
	}

	vec3 cameraPosition = cam->position();
	auto cmp = [cameraPosition, this](const RenderBatchInfo& l, const RenderBatchInfo& r)
	{
		if (l.priority != r.priority)
			return l.priority > r.priority;

		const BlendState& lbs = l.batch->material()->configuration(_main.pass->info().name).blendState;
		const BlendState& rbs = r.batch->material()->configuration(_main.pass->info().name).blendState;
		if (lbs.enabled == rbs.enabled)
		{
			float delta = (l.transformedBox.center - cameraPosition).dotSelf() - (r.transformedBox.center - cameraPosition).dotSelf();
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
