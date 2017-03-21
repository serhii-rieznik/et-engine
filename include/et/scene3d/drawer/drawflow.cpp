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
		for (uint32_t i = 0; i < levelCount; ++i)
			_batches.downsampleBeginInfo.subpasses.emplace_back(0, i);

		TextureDescription::Pointer desc(PointerInit::CreateInplace);
		desc->format = TextureFormat::RGBA16F;
		desc->size = sz;
		desc->isRenderTarget = true;
		desc->levelCount = 1;
		_hdrTarget = _renderer->createTexture(desc);

		uint32_t dowsampledSize = roundToHighestPowerOfTwo(static_cast<uint32_t>(std::min(sz.x, sz.y) / 2));
		desc->format = TextureFormat::RGBA16F;
		desc->size = vec2i(dowsampledSize);
		desc->levelCount = 1;
		while ((dowsampledSize /= 2) >= 1)
			desc->levelCount++;
		_luminanceTarget = _renderer->createTexture(desc);

		_batches.final = renderhelper::createFullscreenRenderBatch(_hdrTarget);
		_batches.downsample = renderhelper::createFullscreenRenderBatch(_hdrTarget, _batches.downsampleMaterial);
		_batches.debug = renderhelper::createFullscreenRenderBatch(_hdrTarget, _batches.debugMaterial);

		RenderPass::ConstructionInfo dsDesc;
		dsDesc.color[0].enabled = true;
		dsDesc.color[0].texture = _luminanceTarget;
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

	downsampleLuminance();

	_finalPass->begin({0, 0});
	_finalPass->pushRenderBatch(_batches.final);

	if (options.debugDraw)
	{
		uint32_t levels = _luminanceTarget->description().levelCount;
		uint32_t gridSize = static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<float>(levels))));
		vec2 vp = vector2ToFloat(_renderer->rc()->size());
		float dx = std::min(vp.y, vp.x) / static_cast<float>(gridSize);
		float dy = dx;

		vec2 pos = vec2(0.0f);
		_batches.debug->material()->setTexture(MaterialTexture::BaseColor, _luminanceTarget);
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

void HDRFlow::downsampleLuminance()
{
	ResourceBarrier barrierToRenderTarget;
	barrierToRenderTarget.toState = TextureState::ColorRenderTarget;

	ResourceBarrier barrierToShaderResource;
	barrierToShaderResource.toState = TextureState::ShaderResource;

	_downsamplePass->begin(_batches.downsampleBeginInfo);
	barrierToRenderTarget.firstLevel = 0;
	_downsamplePass->pushImageBarrier(_luminanceTarget, barrierToRenderTarget);

	_batches.downsample->material()->setTexture(MaterialTexture::BaseColor, _hdrTarget);
	_batches.downsample->material()->setSampler(MaterialTexture::BaseColor, _renderer->clampSampler());
	_batches.downsample->material()->setFloat(MaterialVariable::ExtraParameters, 0.0f);
	_downsamplePass->pushRenderBatch(_batches.downsample);
	_downsamplePass->nextSubpass();

	for (uint32_t level = 1; level < _luminanceTarget->description().levelCount; ++level)
	{
		barrierToShaderResource.firstLevel = level - 1;
		_downsamplePass->pushImageBarrier(_luminanceTarget, barrierToShaderResource);
		barrierToRenderTarget.firstLevel = level;
		_downsamplePass->pushImageBarrier(_luminanceTarget, barrierToRenderTarget);

		_batches.downsample->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(level));
		_batches.downsample->material()->setTexture(MaterialTexture::BaseColor, _luminanceTarget);
		_downsamplePass->pushRenderBatch(_batches.downsample);
		_downsamplePass->nextSubpass();
	}

	barrierToShaderResource.firstLevel = 0;
	barrierToShaderResource.levelCount = _hdrTarget->description().levelCount;
	_downsamplePass->pushImageBarrier(_hdrTarget, barrierToShaderResource);
	_downsamplePass->end();
	_renderer->submitRenderPass(_downsamplePass);
}

}
}
