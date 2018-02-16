/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/drawer/drawflow.h>

namespace et {
namespace s3d {

const TextureFormat HDRTextureFormat = TextureFormat::RGBA32F;

HDRFlow::HDRFlow(RenderInterface::Pointer& ren)
	: _renderer(ren) {
	RenderPass::ConstructionInfo desc(RenderPass::kPassNameDefault);
	desc.color[0].targetClass = RenderTarget::Class::DefaultBuffer;
	desc.color[0].loadOperation = FramebufferOperation::Clear;
	desc.color[0].storeOperation = FramebufferOperation::Store;
	_passes.final = _renderer->allocateRenderPass(desc);

	_materials.debug = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed-lod.json"));
	_materials.posteffects = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/posteffects.json"));

	Sampler::Description smp;
	smp.maxAnisotropy = 1.0f;
	smp.minFilter = TextureFiltration::Linear;
	smp.magFilter = TextureFiltration::Linear;
	smp.wrapU = TextureWrap::ClampToEdge;
	smp.wrapV = TextureWrap::ClampToEdge;
	_colorGradingSampler = _renderer->createSampler(smp);

	setColorGradingTable(emptyString);
	addStep(&HDRFlow::luminanceStep);
	addStep(&HDRFlow::tonemapStep);
	addStep(&HDRFlow::anitialiasStep);
}

void HDRFlow::addStep(PostprocessStep ps) {
	_steps.emplace_back(ps);
}

Texture::Pointer HDRFlow::executeSteps() {

	Texture::Pointer throughput = _primaryTarget;
	for (PostprocessStep ps : _steps)
	{
		Texture::Pointer out = (this->*ps)(throughput);
		throughput = out;
	}
	return throughput;
}

Texture::Pointer HDRFlow::luminanceStep(Texture::Pointer input) {

	if (_lum.batch.invalid() || (_lum.batch->material()->texture(MaterialTexture::BaseColor) != input))
	{
		_lum.batch = renderhelper::createQuadBatch(input, _materials.posteffects, _renderer->clampSampler(), { 0, 1, 0, 1 });
		_lum.batch->material()->setFloat(MaterialVariable::ExtraParameters, 0.0f);
	}

	if (_lum.pass.invalid())
	{
		Texture::Pointer lumTarget;
		{
			TextureDescription::Pointer desc(PointerInit::CreateInplace);
			desc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget | Texture::Flags::CopySource;
			desc->format = TextureFormat::R32F;
			desc->size = vec2i(1024, 1024);
			desc->levelCount = 1;
			lumTarget = _renderer->createTexture(desc);

			desc->size = vec2i(32, 32);
			desc->flags |= Texture::Flags::Storage;
			_lum.downsampled = _renderer->createTexture(desc);
			
			desc->size = vec2i(1);
			_lum.computed = _renderer->createTexture(desc);
		}
		_lum.pass = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("log-luminance", lumTarget));
	}

	if (_lum.downsample.invalid())
	{
		Material::Pointer computeMaterial = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/compute/parallel-reduction.json"));
		_lum.downsample = _renderer->createCompute(computeMaterial);
	}

	if (_lum.adaptation.invalid())
	{
		Material::Pointer computeMaterial = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/compute/luminocity-adaptation.json"));
		_lum.adaptation = _renderer->createCompute(computeMaterial);
	}

	Texture::Pointer luminanceTarget = _lum.pass->colorTarget();

	_lum.pass->begin(RenderPassBeginInfo::singlePass());
	{
		MaterialInstance::Pointer& downsampleMaterial = _lum.downsample->material();
		downsampleMaterial->setTexture(MaterialTexture::BaseColor, luminanceTarget, { 0, 1, 0, 1 });
		downsampleMaterial->setImage(StorageBuffer::StorageBuffer0, _lum.downsampled);
		
		MaterialInstance::Pointer& adaptationMaterial = _lum.adaptation->material();
		adaptationMaterial->setTexture(MaterialTexture::BaseColor, _lum.downsampled, { 0, 1, 0, 1 });
		adaptationMaterial->setImage(StorageBuffer::StorageBuffer0, _lum.computed);

		_lum.pass->pushImageBarrier(luminanceTarget, ResourceBarrier(TextureState::ColorRenderTarget, 0, 1, 0, 1));
		_lum.pass->addSingleRenderBatchSubpass(_lum.batch);

		_lum.pass->pushImageBarrier(luminanceTarget, ResourceBarrier(TextureState::ShaderResource, 0, 1, 0, 1));
		_lum.pass->pushImageBarrier(_lum.downsampled, ResourceBarrier(TextureState::Storage, 0, 1, 0, 1));
		_lum.pass->dispatchCompute(_lum.downsample, vec3i(32, 32, 1));

		_lum.pass->pushImageBarrier(_lum.downsampled, ResourceBarrier(TextureState::ShaderResource, 0, 1, 0, 1));
		_lum.pass->pushImageBarrier(_lum.computed, ResourceBarrier(TextureState::Storage, 0, 1, 0, 1));
		_lum.pass->dispatchCompute(_lum.adaptation, vec3i(1, 1, 1));

		_lum.pass->pushImageBarrier(_lum.downsampled, ResourceBarrier(TextureState::ShaderResource, 0, 1, 0, 1));
		_lum.pass->pushImageBarrier(_lum.computed, ResourceBarrier(TextureState::ShaderResource, 0, 1, 0, 1));
	}
	_lum.pass->end();
	_renderer->submitRenderPass(_lum.pass);
	
	return input;
}

Texture::Pointer HDRFlow::anitialiasStep(Texture::Pointer input) {

	if (_taa.pass.invalid() || _taa.pass->colorTarget()->size(0) != input->size(0))
	{
		TextureDescription::Pointer desc = TextureDescription::Pointer::create(_primaryTarget->description());
		desc->flags |= Texture::Flags::CopySource;
		_taa.pass = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("txaa", _renderer->createTexture(desc)));
	}

	if (_taa.history.invalid() || _taa.history->size(0) != input->size(0))
	{
		TextureDescription::Pointer desc = TextureDescription::Pointer::create(_primaryTarget->description());
		desc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget | Texture::Flags::CopyDestination;
		_taa.history = _renderer->createTexture(desc);
	}

	const Texture::Pointer& target = _taa.pass->colorTarget();
	const Texture::Pointer& vel = drawer()->supportTexture(Drawer::SupportTexture::Velocity);

	if (_taa.batch.invalid() || (_taa.batch->material()->texture(MaterialTexture::BaseColor) != input))
	{
		_taa.batch = renderhelper::createQuadBatch(input, _materials.posteffects, _renderer->clampSampler());
		_taa.batch->material()->setTextureWithSampler(MaterialTexture::EmissiveColor, _taa.history, _renderer->clampSampler());
	}

	_taa.pass->begin(RenderPassBeginInfo::singlePass());
	{
		_taa.batch->material()->setTextureWithSampler(MaterialTexture::Normal, vel, _renderer->clampSampler());
		_taa.pass->setSharedVariable(ObjectVariable::CameraJitter, drawer()->latestCameraJitter());
		_taa.pass->addSingleRenderBatchSubpass(_taa.batch);

		_taa.pass->pushImageBarrier(target, ResourceBarrier(TextureState::CopySource));
		_taa.pass->pushImageBarrier(_taa.history, ResourceBarrier(TextureState::CopyDestination));
		_taa.pass->copyImage(target, _taa.history, CopyDescriptor(vec3i(target->size(0), 1)));
		_taa.pass->pushImageBarrier(target, ResourceBarrier(TextureState::ShaderResource));
		_taa.pass->pushImageBarrier(_taa.history, ResourceBarrier(TextureState::ShaderResource));
	}
	_taa.pass->end();
	_renderer->submitRenderPass(_taa.pass);

	return target;
}

Texture::Pointer HDRFlow::tonemapStep(Texture::Pointer input) {

	if (_tonemap.pass.invalid() || _tonemap.pass->colorTarget()->size(0) != input->size(0))
	{
		TextureDescription::Pointer desc = TextureDescription::Pointer::create(_primaryTarget->description());
		_tonemap.pass = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("tonemapping", _renderer->createTexture(desc)));
	}

	if (_tonemap.batch.invalid())
	{
		_tonemap.batch = renderhelper::createQuadBatch(input, _materials.posteffects, _renderer->clampSampler());
	}

	_tonemap.batch->material()->setTextureWithSampler(MaterialTexture::Shadow, _colorGradingTexture, _colorGradingSampler);
	_tonemap.batch->material()->setTextureWithSampler(MaterialTexture::EmissiveColor, _lum.computed, _renderer->clampSampler());
	_tonemap.pass->executeSingleRenderBatch(_tonemap.batch);
	_renderer->submitRenderPass(_tonemap.pass);

	return _tonemap.pass->colorTarget(0);
}

void HDRFlow::setColorGradingTable(const std::string& st) {
	ObjectsCache localCache;
	if (st.empty())
	{
		_colorGradingTexture = _renderer->loadTexture(application().resolveFileName("engine_data/textures/colorgrading.png"), localCache);
	}
	else
	{
		_colorGradingTexture = _renderer->loadTexture(st, localCache);
	}
}

void HDRFlow::resizeRenderTargets(const vec2i& sz) {
	if (_primaryTarget.invalid() || (_primaryTarget->size(0) != sz))
	{
		TextureDescription::Pointer desc(PointerInit::CreateInplace);
		desc->size = sz;
		desc->format = HDRTextureFormat;
		desc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget;
		desc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget | Texture::Flags::CopySource;
		_primaryTarget = _renderer->createTexture(desc);
	}

	if (drawer().valid())
		drawer()->setRenderTarget(_primaryTarget);
}

void HDRFlow::render() {

	if (drawer().valid())
		drawer()->draw();

	Texture::Pointer processedImage = executeSteps();

	_passes.final->begin(RenderPassBeginInfo::singlePass());
	_passes.final->nextSubpass();
	{
		RenderBatch::Pointer batch = renderhelper::createQuadBatch(processedImage, _materials.posteffects, _renderer->clampSampler());
		_passes.final->pushRenderBatch(batch);
		debugDraw();
	}
	_passes.final->endSubpass();
	_passes.final->end();
	_renderer->submitRenderPass(_passes.final);
}

void HDRFlow::debugDraw() {

	uint32_t gridSize = 4;
	vec2 vp = vector2ToFloat(_renderer->contextSize());
	float dx = std::min(vp.y, vp.x) / static_cast<float>(gridSize);
	float dy = dx;

	vec2 pos = vec2(0.0f);

	auto advancePosition = [&pos, &vp, dx, dy]() {
		pos.x += dx;
		if (pos.x + dx >= vp.x)
		{
			pos.x = 0.0f;
			pos.y += dy;
		}
	};

	RenderBatch::Pointer batch;

	if (options.drawLuminance)
	{
		batch = renderhelper::createQuadBatch(_lum.pass->info().color[0].texture, _materials.debug, _renderer->clampSampler(), renderhelper::QuadType::Default);
		batch->material()->setFloat(MaterialVariable::ExtraParameters, 0.0f);
		_passes.final->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
		_passes.final->pushRenderBatch(batch);
		advancePosition();

		batch = renderhelper::createQuadBatch(_lum.downsampled, _materials.debug, _renderer->clampSampler(), renderhelper::QuadType::Default);
		batch->material()->setFloat(MaterialVariable::ExtraParameters, 0.0f);
		_passes.final->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
		_passes.final->pushRenderBatch(batch);
		advancePosition();

		batch = renderhelper::createQuadBatch(_lum.computed, _materials.debug, _renderer->clampSampler(), renderhelper::QuadType::Default);
		batch->material()->setFloat(MaterialVariable::ExtraParameters, 0.0f);
		_passes.final->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
		_passes.final->pushRenderBatch(batch);
		advancePosition();
	}

	const Texture::Pointer& vel = drawer()->supportTexture(Drawer::SupportTexture::Velocity);
	if (options.drawVelocity && vel.valid())
	{
		Material::Pointer m = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d.json"));
		batch = renderhelper::createQuadBatch(vel, m, renderhelper::QuadType::Default);
		_passes.final->pushRenderBatch(batch);
		advancePosition();
	}

	const Texture::Pointer& sss = drawer()->supportTexture(Drawer::SupportTexture::ScreenspaceShadows);
	if (options.drawShadows && sss.valid())
	{
		Material::Pointer m = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d.json"));
		batch = renderhelper::createQuadBatch(sss, m, renderhelper::QuadType::Default);
		_passes.final->pushRenderBatch(batch);
		advancePosition();
	}

	const Texture::Pointer& ssao = drawer()->supportTexture(Drawer::SupportTexture::ScreenspaceAO);
	if (options.drawAO && ssao.valid())
	{
		Material::Pointer m = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d.json"));
		batch = renderhelper::createQuadBatch(ssao, m, renderhelper::QuadType::Default);
		_passes.final->pushRenderBatch(batch);
		advancePosition();
	}
}

}
}
