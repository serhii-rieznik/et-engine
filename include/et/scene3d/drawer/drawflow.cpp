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
	desc.name = RenderPass::kPassNameDefault;
	desc.color[0].enabled = true;
	desc.color[0].loadOperation = FramebufferOperation::Clear;
	desc.color[0].storeOperation = FramebufferOperation::Store;
	_passes.final = _renderer->allocateRenderPass(desc);
	
	/*
	 * TEST COMPUTE
	 *
	_materials.computeTest = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/compute/test.json"));
	_testCompute = _renderer->createCompute(_materials.computeTest);
	// */

	_materials.debug = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed-lod.json"));
	_materials.posteffects = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/posteffects.json"));

	/*
	_batches.debug = renderhelper::createFullscreenRenderBatch(Texture::Pointer(), _materials.debug);
	_batches.final = renderhelper::createFullscreenRenderBatch(Texture::Pointer(), _materials.posteffects);
	_batches.tonemap = renderhelper::createFullscreenRenderBatch(Texture::Pointer(), _materials.posteffects);
	_batches.downsample = renderhelper::createFullscreenRenderBatch(Texture::Pointer(), _materials.posteffects);
	_batches.motionBlur = renderhelper::createFullscreenRenderBatch(Texture::Pointer(), _materials.posteffects);
	_batches.txaa = renderhelper::createFullscreenRenderBatch(Texture::Pointer(), _materials.posteffects);
	*/
}

void HDRFlow::resizeRenderTargets(const vec2i& sz)
{
	if (_primaryTarget.invalid() || (_primaryTarget->size(0) != sz))
	{
		uint32_t levelCount = 1;
		uint32_t minDimension = static_cast<uint32_t>(std::min(sz.x, sz.y));
		while ((minDimension /= 2) > 0)
			++levelCount;

		_passes.logLuminanceBeginInfo.subpasses.emplace_back(0, 0);

		for (uint32_t i = 1; i + 1  < levelCount; ++i)
			_passes.averageLuminanceBeginInfo.subpasses.emplace_back(0, i);

		_passes.resolveLuminanceBeginInfo.subpasses.emplace_back(0, levelCount - 1);

		TextureDescription::Pointer desc(PointerInit::CreateInplace);
		desc->size = sz;
		desc->format = HDRTextureFormat;
		desc->flags = Texture::Flags::RenderTarget | Texture::Flags::Storage;
		_secondaryTarget = _renderer->createTexture(desc);
		desc->flags = Texture::Flags::RenderTarget | Texture::Flags::CopySource;
		_primaryTarget = _renderer->createTexture(desc);
		desc->flags = Texture::Flags::RenderTarget | Texture::Flags::CopyDestination;
		_renderHistory = _renderer->createTexture(desc);

		desc->size = vec2i(256, 1);
		desc->flags = Texture::Flags::Storage;
		_luminanceHistogram = _renderer->createTexture(desc);

		uint32_t downsampledSize = roundToHighestPowerOfTwo(static_cast<uint32_t>(std::min(sz.x, sz.y) / 2));
		desc->size = vec2i(downsampledSize);
		desc->flags = Texture::Flags::RenderTarget | Texture::Flags::CopySource;
		desc->levelCount = 1;
		while ((downsampledSize /= 2) >= 1) desc->levelCount++;
		_luminanceTarget = _renderer->createTexture(desc);

		desc->levelCount = 1;
		desc->size = vec2i(1);
		desc->flags = Texture::Flags::RenderTarget | Texture::Flags::CopyDestination;
		_luminanceHistory = _renderer->createTexture(desc);

		_passes.logLuminance = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("log-luminance", _luminanceTarget));
		_passes.averageLuminance = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("average-luminance", _luminanceTarget));
		_passes.resolveLuminance = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("resolve-luminance", _luminanceTarget));

		_passes.motionBlur0 = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("motionblur", _secondaryTarget));
		_passes.motionBlur1 = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("motionblur", _primaryTarget));
		_passes.tonemapping = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("tonemapping", _secondaryTarget));
		_passes.txaa = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("txaa", _primaryTarget));
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
	tonemap();
	antialias();

	_passes.final->begin(RenderPassBeginInfo::singlePass());
	_passes.final->nextSubpass();
	{
		RenderBatch::Pointer batch = renderhelper::createFullscreenRenderBatch(_primaryTarget, _materials.posteffects, _renderer->clampSampler());
		_passes.final->pushRenderBatch(batch);
		debugDraw();
	}
	_passes.final->endSubpass();
	_passes.final->end();
	_renderer->submitRenderPass(_passes.final);
}

void HDRFlow::postprocess()
{
	const Texture::Pointer& vel = drawer()->supportTexture(Drawer::SupportTexture::Velocity);
	RenderBatch::Pointer batch = renderhelper::createFullscreenRenderBatch(_primaryTarget, _materials.posteffects, _renderer->clampSampler());
	batch->material()->setTextureWithSampler(MaterialTexture::EmissiveColor, vel, _renderer->clampSampler());
	
	_passes.motionBlur0->begin(RenderPassBeginInfo::singlePass());
	_passes.motionBlur0->nextSubpass();
	_passes.motionBlur0->pushRenderBatch(batch);
	_passes.motionBlur0->endSubpass();
	_passes.motionBlur0->end();
	
	_renderer->submitRenderPass(_passes.motionBlur0);
	
	batch->material()->setTextureWithSampler(MaterialTexture::BaseColor, _secondaryTarget, _renderer->clampSampler());
	_passes.motionBlur1->begin(RenderPassBeginInfo::singlePass());
	_passes.motionBlur1->nextSubpass();
	_passes.motionBlur1->pushRenderBatch(batch);
	_passes.motionBlur1->endSubpass();
	_passes.motionBlur1->end();
	_renderer->submitRenderPass(_passes.motionBlur1);
}

void HDRFlow::downsampleLuminance()
{
	{
		RenderBatch::Pointer logLuminanceBatch = renderhelper::createFullscreenRenderBatch(_primaryTarget, _materials.posteffects, _renderer->clampSampler(), { 0, 1, 0, 1 });
		logLuminanceBatch->material()->setFloat(MaterialVariable::ExtraParameters, 0.0f);

		_passes.logLuminance->begin(_passes.logLuminanceBeginInfo);
		_passes.logLuminance->pushImageBarrier(_luminanceTarget, ResourceBarrier(TextureState::ColorRenderTarget, 0, 1, 0, 1));

		_passes.logLuminance->nextSubpass();
		_passes.logLuminance->pushRenderBatch(logLuminanceBatch);
		_passes.logLuminance->endSubpass();
		
		_passes.logLuminance->end();
		_renderer->submitRenderPass(_passes.logLuminance);
	}
	
	uint32_t processedLevel = 1;
	{
		_passes.averageLuminance->begin(_passes.averageLuminanceBeginInfo);
		for (; processedLevel + 1 < _luminanceTarget->description().levelCount; ++processedLevel)
		{
			RenderBatch::Pointer averageLuminanceBatch = renderhelper::createFullscreenRenderBatch(_luminanceTarget, _materials.posteffects, _renderer->clampSampler(), { 0, processedLevel, 0, 1 });
			averageLuminanceBatch->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(processedLevel));

			_passes.averageLuminance->pushImageBarrier(_luminanceTarget, ResourceBarrier(TextureState::ColorRenderTarget, processedLevel, 1, 0, 1));
			_passes.averageLuminance->pushImageBarrier(_luminanceTarget, ResourceBarrier(TextureState::ShaderResource, 0, processedLevel, 0, 1));
			_passes.averageLuminance->nextSubpass();
			_passes.averageLuminance->pushRenderBatch(averageLuminanceBatch);
			_passes.averageLuminance->endSubpass();
		}
		_passes.averageLuminance->end();
		_renderer->submitRenderPass(_passes.averageLuminance);
	}

	{
		RenderBatch::Pointer resolveLuminanceBatch = renderhelper::createFullscreenRenderBatch(_luminanceTarget, _materials.posteffects, _renderer->clampSampler(), { 0, processedLevel, 0, 1 });
		resolveLuminanceBatch->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(processedLevel));
		resolveLuminanceBatch->material()->setTextureWithSampler(MaterialTexture::Shadow, _luminanceHistory, _renderer->clampSampler(), { 0, 1, 0, 1 });

		CopyDescriptor copyLuminance;
		copyLuminance.levelFrom = _luminanceTarget->description().levelCount - 1;
		copyLuminance.size = vec3i(1, 1, 1);

		_passes.resolveLuminance->begin(_passes.resolveLuminanceBeginInfo);

		_passes.resolveLuminance->pushImageBarrier(_luminanceTarget, ResourceBarrier(TextureState::ColorRenderTarget, processedLevel, 1, 0, 1));
		_passes.resolveLuminance->pushImageBarrier(_luminanceTarget, ResourceBarrier(TextureState::ShaderResource, 0, processedLevel, 0, 1));
		_passes.resolveLuminance->nextSubpass();
		_passes.resolveLuminance->pushRenderBatch(resolveLuminanceBatch);
		_passes.resolveLuminance->endSubpass();
		
		_passes.resolveLuminance->pushImageBarrier(_luminanceTarget, ResourceBarrier(TextureState::CopySource, _luminanceTarget->description().levelCount - 1, 0));
		_passes.resolveLuminance->pushImageBarrier(_luminanceHistory, ResourceBarrier(TextureState::CopyDestination));
		_passes.resolveLuminance->copyImage(_luminanceTarget, _luminanceHistory, copyLuminance);

		_passes.resolveLuminance->pushImageBarrier(_primaryTarget, ResourceBarrier(TextureState::ShaderResource, 0, _primaryTarget->description().levelCount, 0, 1));
		_passes.resolveLuminance->pushImageBarrier(_luminanceTarget, ResourceBarrier(TextureState::ShaderResource, 0, _luminanceTarget->description().levelCount, 0, 1));
		_passes.resolveLuminance->pushImageBarrier(_luminanceHistory, ResourceBarrier(TextureState::ShaderResource, 0, 1, 0, 1));
		_passes.resolveLuminance->end();

		_renderer->submitRenderPass(_passes.resolveLuminance);
	}
	
}

void HDRFlow::tonemap()
{
	RenderBatch::Pointer batch = renderhelper::createFullscreenRenderBatch(_primaryTarget, _materials.posteffects, _renderer->clampSampler());
	batch->material()->setTextureWithSampler(MaterialTexture::EmissiveColor, _luminanceTarget, _renderer->clampSampler());

	_passes.tonemapping->begin(RenderPassBeginInfo::singlePass());
	_passes.tonemapping->nextSubpass();
	_passes.tonemapping->pushRenderBatch(batch);
	_passes.tonemapping->endSubpass();
	_passes.tonemapping->end();
	
	/*
	 * TEST COMPUTE
	 *
	_passes.tonemapping->pushImageBarrier(_luminanceHistogram, ResourceBarrier(TextureState::Storage));
	_passes.tonemapping->pushImageBarrier(_secondaryTarget, ResourceBarrier(TextureState::ShaderResource));
	_testCompute->material()->setTexture(MaterialTexture::BaseColor, _secondaryTarget);
	_testCompute->material()->setImage(StorageBuffer::StorageBuffer0, _luminanceHistogram);
	_passes.tonemapping->dispatchCompute(_testCompute, vec3i(_secondaryTarget->size(0), 1));
	_passes.tonemapping->pushImageBarrier(_luminanceHistogram, ResourceBarrier(TextureState::ShaderResource));
	// */
	
	_renderer->submitRenderPass(_passes.tonemapping);
}

void HDRFlow::antialias()
{
	const Texture::Pointer& vel = drawer()->supportTexture(Drawer::SupportTexture::Velocity);

	RenderBatch::Pointer batch = renderhelper::createFullscreenRenderBatch(_secondaryTarget, _materials.posteffects, _renderer->clampSampler());
	batch->material()->setTextureWithSampler(MaterialTexture::Normal, vel, _renderer->clampSampler());
	batch->material()->setTextureWithSampler(MaterialTexture::EmissiveColor, _renderHistory, _renderer->clampSampler());

	_passes.txaa->setSharedVariable(ObjectVariable::CameraJitter, drawer()->latestCameraJitter());
	_passes.txaa->begin(RenderPassBeginInfo::singlePass());
	_passes.txaa->nextSubpass();
	_passes.txaa->pushRenderBatch(batch);
	_passes.txaa->endSubpass();

	CopyDescriptor copyDescriptor;
	copyDescriptor.size = vec3i(_primaryTarget->size(0), 1);

	_passes.txaa->pushImageBarrier(_primaryTarget, ResourceBarrier(TextureState::CopySource));
	_passes.txaa->pushImageBarrier(_renderHistory, ResourceBarrier(TextureState::CopyDestination));
	_passes.txaa->copyImage(_primaryTarget, _renderHistory, copyDescriptor);
	_passes.txaa->pushImageBarrier(_primaryTarget, ResourceBarrier(TextureState::ShaderResource));
	_passes.txaa->pushImageBarrier(_renderHistory, ResourceBarrier(TextureState::ShaderResource));

	_passes.txaa->end();
	_renderer->submitRenderPass(_passes.txaa);
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
	RenderBatch::Pointer batch = renderhelper::createFullscreenRenderBatch(_luminanceTarget, _materials.debug, _renderer->clampSampler());
	for (uint32_t i = 0; i < levels; ++i)
	{
		batch->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(i));

		_passes.final->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
		_passes.final->pushRenderBatch(batch);
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
		_passes.final->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, vec2(0.0f, 0.5f * vp.y), 0.5f * vp));
		_passes.final->pushRenderBatch(b);
	}

	{
		Material::Pointer m = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed.json"));
		RenderBatch::Pointer b = renderhelper::createFullscreenRenderBatch(_luminanceHistogram, m);
		_passes.final->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, vec2(0.0f, 0.5f * vp.y), vec2(256.0f, 32.0f)));
		_passes.final->pushRenderBatch(b);
	}
}

}
}
