/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/drawer/drawflow.h>

namespace et
{
namespace s3d
{

HDRFlow::HDRFlow(const RenderInterface::Pointer& ren) : 
	_renderer(ren)
{
	RenderPass::ConstructionInfo desc;
	desc.color[0].loadOperation = FramebufferOperation::DontCare;
	desc.color[0].useDefaultRenderTarget = true;
	desc.color[0].enabled = true;
	desc.name = RenderPass::kPassNameDefault;
	_finalPass = _renderer->allocateRenderPass(desc);
	
	_batches.downsampleMaterial = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/posteffects.json"));
	_batches.debugMaterial = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2dtransformedlod.json"));
}

void HDRFlow::resizeRenderTargets(const vec2i& sz)
{
	if (_hdrTarget.invalid() || (_hdrTarget->size(0) != sz))
	{
		uint32_t levelCount = 1;
		uint32_t minDimension = static_cast<uint32_t>(std::min(sz.x, sz.y));
		while ((minDimension /= 2) > 0)
			++levelCount;

		_batches.downsampleBeginInfo.subpasses.clear();
		for (uint32_t i = 1; i < levelCount; ++i)
			_batches.downsampleBeginInfo.subpasses.emplace_back(0, i);

		TextureDescription::Pointer desc(PointerInit::CreateInplace);
		desc->format = TextureFormat::RGBA16F;
		desc->size = sz;
		desc->isRenderTarget = true;
		desc->levelCount = levelCount;
		_hdrTarget = _renderer->createTexture(desc);

		_batches.final = renderhelper::createFullscreenRenderBatch(_hdrTarget);

		_batches.downsample = renderhelper::createFullscreenRenderBatch(_hdrTarget, _batches.downsampleMaterial);
		_batches.debug = renderhelper::createFullscreenRenderBatch(_hdrTarget, _batches.debugMaterial);

		RenderPass::ConstructionInfo dsDesc;
		dsDesc.color[0].enabled = true;
		dsDesc.color[0].texture = _hdrTarget;
		dsDesc.color[0].useDefaultRenderTarget = false;
		dsDesc.color[0].loadOperation = FramebufferOperation::Clear;
		dsDesc.name = "downsample";
		_downsamplePass = _renderer->allocateRenderPass(dsDesc);
	}

	if (drawer().valid())
		drawer()->setRenderTarget(_hdrTarget);
}

void HDRFlow::render()
{
	if (drawer().valid())
		drawer()->draw();

	ResourceBarrier barrierToRenderTarget;
	// barrierToRenderTarget.stateFrom = TextureState::ShaderResource;
	barrierToRenderTarget.toState = TextureState::ColorRenderTarget;
	
	ResourceBarrier barrierToShaderResource;
	// barrierToShaderResource.stateFrom = TextureState::ColorRenderTarget;
	barrierToShaderResource.toState = TextureState::ShaderResource;

	_downsamplePass->begin(_batches.downsampleBeginInfo);
	_batches.downsample->material()->setTexture(MaterialTexture::BaseColor, _hdrTarget);

	for (uint32_t l = 1; l < _hdrTarget->description().levelCount; ++l)
	{
		_batches.downsample->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(l));
		
		barrierToShaderResource.firstLevel = l - 1;
		_downsamplePass->pushImageBarrier(_hdrTarget, barrierToShaderResource);
		barrierToRenderTarget.firstLevel = l;
		_downsamplePass->pushImageBarrier(_hdrTarget, barrierToRenderTarget);

		_downsamplePass->pushRenderBatch(_batches.downsample);
		_downsamplePass->nextSubpass();
	}
	barrierToShaderResource.firstLevel = 0;
	barrierToShaderResource.levelCount = _hdrTarget->description().levelCount;
	_downsamplePass->pushImageBarrier(_hdrTarget, barrierToShaderResource);
	_downsamplePass->end();
	_renderer->submitRenderPass(_downsamplePass);

	_finalPass->begin({0, 0});
	_finalPass->pushRenderBatch(_batches.final);

	if (options.debugDraw)
	{
		uint32_t levels = _hdrTarget->description().levelCount;
		uint32_t gridSize = static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<float>(levels))));

		vec2 vp = vector2ToFloat(_renderer->rc()->size());
		float dy = vp.y / static_cast<float>(gridSize);
		float dx = _hdrTarget->sizeFloat(0).aspect() * dy;
		
		vec2 pos = vec2(0.0f, 0.0f);
		_batches.debug->material()->setTexture(MaterialTexture::BaseColor, _hdrTarget);
		for (uint32_t i = 0; i < levels; ++i)
		{
			_batches.debug->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(i));
			_batches.debug->setTransformation(fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
			_finalPass->pushRenderBatch(_batches.debug);
			pos.x += dx;
			if (pos.x + dx >= vp.x)
			{
				pos.x = 0.0f;
				pos.y += dy;
			}
		}
	}

	_finalPass->end();
	_renderer->submitRenderPass(_finalPass);
}

}
}
