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

const TextureFormat HDRTextureFormat = TextureFormat::RGBA32F;

HDRFlow::HDRFlow(const RenderInterface::Pointer& ren) : 
	_renderer(ren)
{
	RenderPass::ConstructionInfo desc;
	desc.color[0].loadOperation = FramebufferOperation::DontCare;
	desc.color[0].useDefaultRenderTarget = true;
	desc.color[0].enabled = true;
	desc.name = RenderPass::kPassNameDefault;
	_finalPass = _renderer->allocateRenderPass(desc);
	
	_materials.debug = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed-lod.json"));
	_materials.posteffects = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/posteffects.json"));
	_batches.debug = renderhelper::createFullscreenRenderBatch(Texture::Pointer(), _materials.debug);
	_batches.final = renderhelper::createFullscreenRenderBatch(Texture::Pointer(), _materials.posteffects);
	_batches.downsample = renderhelper::createFullscreenRenderBatch(Texture::Pointer(), _materials.posteffects);
	_batches.motionBlur = renderhelper::createFullscreenRenderBatch(Texture::Pointer(), _materials.posteffects);
}

void HDRFlow::resizeRenderTargets(const vec2i& sz)
{
	if (_primaryTarget.invalid() || (_primaryTarget->size(0) != sz))
	{
		uint32_t levelCount = 1;
		uint32_t minDimension = static_cast<uint32_t>(std::min(sz.x, sz.y));
		while ((minDimension /= 2) > 0)
			++levelCount;

		_batches.downsampleBeginInfo.subpasses.clear();
		for (uint32_t i = 0; i < levelCount; ++i)
			_batches.downsampleBeginInfo.subpasses.emplace_back(0, i);

		TextureDescription::Pointer desc(PointerInit::CreateInplace);
		desc->format = HDRTextureFormat;
		desc->size = sz;
		desc->flags = Texture::Flags::RenderTarget;
		desc->levelCount = 1;
		_primaryTarget = _renderer->createTexture(desc);
		_secondaryTarget = _renderer->createTexture(desc);

		uint32_t downsampledSize = roundToHighestPowerOfTwo(static_cast<uint32_t>(std::min(sz.x, sz.y) / 2));
		desc->size = vec2i(downsampledSize);
		desc->levelCount = 1;
		desc->flags = Texture::Flags::RenderTarget | Texture::Flags::CopySource;
		while ((downsampledSize /= 2) >= 1)
			desc->levelCount++;
		_luminanceTarget = _renderer->createTexture(desc);

		desc->size = vec2i(1);
		desc->levelCount = 1;
		desc->flags = Texture::Flags::RenderTarget | Texture::Flags::CopyDestination;
		_luminanceHistory = _renderer->createTexture(desc);

		RenderPass::ConstructionInfo dsDesc;
		dsDesc.color[0].enabled = true;
		dsDesc.color[0].texture = _luminanceTarget;
		dsDesc.color[0].useDefaultRenderTarget = false;
		dsDesc.color[0].loadOperation = FramebufferOperation::Clear;
		dsDesc.name = "downsample";
		_downsamplePass = _renderer->allocateRenderPass(dsDesc);

		RenderPass::ConstructionInfo mbDesc;
		mbDesc.color[0].enabled = true;
		mbDesc.color[0].useDefaultRenderTarget = false;
		mbDesc.color[0].loadOperation = FramebufferOperation::Clear;
		mbDesc.name = "motionblur";
		for (uint32_t i = 0; i < MotionBlurPassCount; ++i)
		{
			mbDesc.color[0].texture = (i % 2 == 0) ? _secondaryTarget : _primaryTarget;
			_motionBlurPass[i]= _renderer->allocateRenderPass(mbDesc);
		}
	}
	
	if (drawer().valid())
		drawer()->setRenderTarget(_primaryTarget);
}

void HDRFlow::render()
{
	if (drawer().valid())
		drawer()->draw();

	postprocess();
	downsampleLuminance();

	_finalPass->begin(RenderPassBeginInfo::singlePass);
	{
		_batches.final->material()->setTextureWithSampler(MaterialTexture::BaseColor, _primaryTarget, _renderer->clampSampler());
		_batches.final->material()->setTextureWithSampler(MaterialTexture::EmissiveColor, _luminanceTarget, _renderer->clampSampler());
		_finalPass->pushRenderBatch(_batches.final);
		debugDraw();
	}
	_finalPass->end();
	_renderer->submitRenderPass(_finalPass);
}

void HDRFlow::postprocess()
{
	const Texture::Pointer& vel = drawer()->supportTexture(Drawer::SupportTexture::Velocity);
	_batches.motionBlur->material()->setTextureWithSampler(MaterialTexture::EmissiveColor, vel, _renderer->clampSampler());

	for (uint32_t i = 0; i < MotionBlurPassCount; ++i)
	{
		Texture::Pointer src = (i % 2 == 0) ? _primaryTarget : _secondaryTarget;
		_batches.motionBlur->material()->setTextureWithSampler(MaterialTexture::BaseColor, src, _renderer->clampSampler());

		_motionBlurPass[i]->begin(RenderPassBeginInfo::singlePass);
		_motionBlurPass[i]->pushRenderBatch(_batches.motionBlur);
		_motionBlurPass[i]->end();
		_renderer->submitRenderPass(_motionBlurPass[i]);
	}
}

void HDRFlow::downsampleLuminance()
{
	ResourceBarrier barrierToRenderTarget(TextureState::ColorRenderTarget);
	ResourceBarrier barrierToShaderResource(TextureState::ShaderResource);

	_downsamplePass->begin(_batches.downsampleBeginInfo);
	barrierToRenderTarget.firstLevel = 0;
	_downsamplePass->pushImageBarrier(_luminanceTarget, barrierToRenderTarget);

	_batches.downsample->material()->setTextureWithSampler(MaterialTexture::Shadow, _luminanceHistory, _renderer->clampSampler());
	_batches.downsample->material()->setTextureWithSampler(MaterialTexture::BaseColor, _primaryTarget, _renderer->clampSampler());
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
		_batches.downsample->material()->setTextureWithSampler(MaterialTexture::BaseColor, _luminanceTarget, _renderer->clampSampler());
		_downsamplePass->pushRenderBatch(_batches.downsample);
		_downsamplePass->nextSubpass();
	}
	_downsamplePass->end();

	ResourceBarrier toCopySource(TextureState::CopySource);
	toCopySource.firstLevel = _luminanceTarget->description().levelCount - 1;
	toCopySource.levelCount = 1;
	_downsamplePass->pushImageBarrier(_luminanceTarget, toCopySource);

	ResourceBarrier toCopyDestination(TextureState::CopyDestination);
	toCopyDestination.firstLevel = 0;
	toCopyDestination.levelCount = 1;
	_downsamplePass->pushImageBarrier(_luminanceHistory, toCopyDestination);
	
	CopyDescriptor copyLuminance;
	copyLuminance.levelFrom = _luminanceTarget->description().levelCount - 1;
	copyLuminance.size = vec3i(1, 1, 1);
	_downsamplePass->copyImage(_luminanceTarget, _luminanceHistory, copyLuminance);

	barrierToShaderResource.firstLevel = 0;
	barrierToShaderResource.levelCount = _primaryTarget->description().levelCount;
	_downsamplePass->pushImageBarrier(_primaryTarget, barrierToShaderResource);
	
	barrierToShaderResource.levelCount = _luminanceTarget->description().levelCount;
	_downsamplePass->pushImageBarrier(_luminanceTarget, barrierToShaderResource);

	barrierToShaderResource.levelCount = 1;
	_downsamplePass->pushImageBarrier(_luminanceHistory, barrierToShaderResource);

	_renderer->submitRenderPass(_downsamplePass);
}

void HDRFlow::debugDraw()
{
	if (!options.debugDraw)
		return;

	uint32_t levels = _luminanceTarget->description().levelCount;
	uint32_t gridSize = static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<float>(levels))));
	vec2 vp = vector2ToFloat(_renderer->rc()->size());
	float dx = std::min(vp.y, vp.x) / static_cast<float>(gridSize);
	float dy = dx;

	vec2 pos = vec2(0.0f);
	_batches.debug->material()->setTextureWithSampler(MaterialTexture::BaseColor, _luminanceTarget, _renderer->clampSampler());
	for (uint32_t i = 0; i < levels; ++i)
	{
		_batches.debug->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(i));
		_finalPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
		_finalPass->pushRenderBatch(_batches.debug);
		pos.x += dx;
		if (pos.x + dx >= vp.x)
		{
			pos.x = 0.0f;
			pos.y += dy;
		}
	}

	const Texture::Pointer& vel = drawer()->supportTexture(Drawer::SupportTexture::Velocity);
	if (vel.valid())
	{
		Material::Pointer m = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed.json"));
		RenderBatch::Pointer b = renderhelper::createFullscreenRenderBatch(vel, m);
		_finalPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, vec2(0.0f, 0.5f * vp.y), 0.5f * vp));
		_finalPass->pushRenderBatch(b);
	}
}

}
}
