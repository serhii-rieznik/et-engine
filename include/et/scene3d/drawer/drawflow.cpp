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
const uint32_t luminanceTargetSize = 1024;
const uint32_t computeGroupSize = 16;

HDRFlow::HDRFlow(const RenderInterface::Pointer& ren)
	: _renderer(ren) {
	RenderPass::ConstructionInfo desc;
	desc.name = RenderPass::kPassNameDefault;
	desc.color[0].targetClass = RenderTarget::Class::DefaultBuffer;
	desc.color[0].loadOperation = FramebufferOperation::Clear;
	desc.color[0].storeOperation = FramebufferOperation::Store;
	_passes.final = _renderer->allocateRenderPass(desc);

	_materials.debug = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed-lod.json"));
	_materials.posteffects = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/posteffects.json"));

	{
		Material::Pointer computeMaterial;

		computeMaterial = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/compute/parallel-reduction.json"));
		_compute.downsampleLuminance = _renderer->createCompute(computeMaterial);

		computeMaterial = _renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/compute/luminocity-adaptation.json"));
		_compute.luminanceAdaptation = _renderer->createCompute(computeMaterial);
	}

	Sampler::Description smp;
	smp.maxAnisotropy = 1.0f;
	smp.minFilter = TextureFiltration::Linear;
	smp.magFilter = TextureFiltration::Linear;
	smp.wrapU = TextureWrap::ClampToEdge;
	smp.wrapV = TextureWrap::ClampToEdge;
	_colorGradingSampler = _renderer->createSampler(smp);

	setColorGradingTable(emptyString);
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
		_secondaryTarget = _renderer->createTexture(desc);
		desc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget | Texture::Flags::CopySource;
		_primaryTarget = _renderer->createTexture(desc);
		desc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget | Texture::Flags::CopyDestination;
		_renderHistory = _renderer->createTexture(desc);

		desc->size = vec2i(luminanceTargetSize);
		desc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget | Texture::Flags::CopySource;
		desc->levelCount = 1;

		_passes.logLuminanceBeginInfo.subpasses.clear();
		_passes.logLuminanceBeginInfo.subpasses.emplace_back(0, 0);

		desc->format = TextureFormat::R32F;

		desc->levelCount = 1;
		_luminanceTarget = _renderer->createTexture(desc);

		desc->flags = Texture::Flags::ShaderResource | Texture::Flags::Storage;

		desc->size = vec2i(32, 32);
		_downsampledLuminance = _renderer->createTexture(desc);

		desc->size = vec2i(1, 1);
		_computedLuminance = _renderer->createTexture(desc);

		_passes.logLuminance = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("log-luminance", _luminanceTarget));

		_passes.motionBlur0 = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("motionblur", _secondaryTarget));
		_passes.motionBlur1 = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("motionblur", _primaryTarget));
		_passes.tonemapping = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("tonemapping", _secondaryTarget));
		_passes.txaa = _renderer->allocateRenderPass(RenderPass::renderTargetPassInfo("txaa", _primaryTarget));

		_batches.logLuminance = renderhelper::createQuadBatch(_primaryTarget, _materials.posteffects, _renderer->clampSampler(), { 0, 1, 0, 1 });
		_batches.logLuminance->material()->setFloat(MaterialVariable::ExtraParameters, 0.0f);
	}

	if (drawer().valid())
		drawer()->setRenderTarget(_primaryTarget);
}

void HDRFlow::render() {
	if (drawer().valid())
		drawer()->draw();

	postprocess();
	downsampleLuminance();
	tonemap();
	antialias();

	_passes.final->begin(RenderPassBeginInfo::singlePass());
	_passes.final->nextSubpass();
	{
		RenderBatch::Pointer batch = renderhelper::createQuadBatch(_primaryTarget, _materials.posteffects, _renderer->clampSampler());
		_passes.final->pushRenderBatch(batch);
		debugDraw();
	}
	_passes.final->endSubpass();
	_passes.final->end();
	_renderer->submitRenderPass(_passes.final);
}

void HDRFlow::postprocess() {
	const Texture::Pointer& vel = drawer()->supportTexture(Drawer::SupportTexture::Velocity);
	RenderBatch::Pointer batch = renderhelper::createQuadBatch(_primaryTarget, _materials.posteffects, _renderer->clampSampler());
	batch->material()->setTextureWithSampler(MaterialTexture::Normal, vel, _renderer->clampSampler());

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

void HDRFlow::downsampleLuminance() {
	_passes.logLuminance->begin(_passes.logLuminanceBeginInfo);
	_passes.logLuminance->pushImageBarrier(_luminanceTarget, ResourceBarrier(TextureState::ColorRenderTarget, 0, 1, 0, 1));

	_passes.logLuminance->nextSubpass();
	_passes.logLuminance->pushRenderBatch(_batches.logLuminance);
	_passes.logLuminance->endSubpass();

	uint32_t dispatchSize = 32;

	MaterialInstance::Pointer& downsampleMaterial = _compute.downsampleLuminance->material();
	_passes.logLuminance->pushImageBarrier(_luminanceTarget, ResourceBarrier(TextureState::ShaderResource, 0, 1, 0, 1));
	_passes.logLuminance->pushImageBarrier(_downsampledLuminance, ResourceBarrier(TextureState::Storage, 0, 1, 0, 1));
	downsampleMaterial->setTexture(MaterialTexture::BaseColor, _luminanceTarget, { 0, 1, 0, 1 });
	downsampleMaterial->setImage(StorageBuffer::StorageBuffer0, _downsampledLuminance);
	_passes.logLuminance->dispatchCompute(_compute.downsampleLuminance, vec3i(dispatchSize, dispatchSize, 1));

	MaterialInstance::Pointer& adaptationMaterial = _compute.luminanceAdaptation->material();
	adaptationMaterial->setTexture(MaterialTexture::BaseColor, _downsampledLuminance, { 0, 1, 0, 1 });
	adaptationMaterial->setImage(StorageBuffer::StorageBuffer0, _computedLuminance);
	_passes.logLuminance->pushImageBarrier(_downsampledLuminance, ResourceBarrier(TextureState::ShaderResource, 0, 1, 0, 1));
	_passes.logLuminance->pushImageBarrier(_computedLuminance, ResourceBarrier(TextureState::Storage, 0, 1, 0, 1));
	_passes.logLuminance->dispatchCompute(_compute.luminanceAdaptation, vec3i(1, 1, 1));

	_passes.logLuminance->pushImageBarrier(_downsampledLuminance, ResourceBarrier(TextureState::ShaderResource, 0, 1, 0, 1));
	_passes.logLuminance->pushImageBarrier(_computedLuminance, ResourceBarrier(TextureState::ShaderResource, 0, 1, 0, 1));

	_passes.logLuminance->end();
	_renderer->submitRenderPass(_passes.logLuminance);
}

void HDRFlow::tonemap() {
	RenderBatch::Pointer batch = renderhelper::createQuadBatch(_primaryTarget, _materials.posteffects, _renderer->clampSampler());
	batch->material()->setTextureWithSampler(MaterialTexture::Shadow, _colorGradingTexture, _colorGradingSampler);
	batch->material()->setTextureWithSampler(MaterialTexture::EmissiveColor, _computedLuminance, _renderer->clampSampler());

	_passes.tonemapping->begin(RenderPassBeginInfo::singlePass());
	_passes.tonemapping->nextSubpass();
	_passes.tonemapping->pushRenderBatch(batch);
	_passes.tonemapping->endSubpass();
	_passes.tonemapping->end();

	_renderer->submitRenderPass(_passes.tonemapping);
}

void HDRFlow::antialias() {
	const Texture::Pointer& vel = drawer()->supportTexture(Drawer::SupportTexture::Velocity);

	RenderBatch::Pointer batch = renderhelper::createQuadBatch(_secondaryTarget, _materials.posteffects, _renderer->clampSampler());
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

void HDRFlow::debugDraw() {

	uint32_t gridSize = 4;
	vec2 vp = vector2ToFloat(_renderer->rc()->size());
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
		batch = renderhelper::createQuadBatch(_luminanceTarget, _materials.debug, _renderer->clampSampler(), renderhelper::QuadType::Default);
		batch->material()->setFloat(MaterialVariable::ExtraParameters, 0.0f);
		_passes.final->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
		_passes.final->pushRenderBatch(batch);
		advancePosition();

		batch = renderhelper::createQuadBatch(_downsampledLuminance, _materials.debug, _renderer->clampSampler(), renderhelper::QuadType::Default);
		batch->material()->setFloat(MaterialVariable::ExtraParameters, 0.0f);
		_passes.final->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
		_passes.final->pushRenderBatch(batch);
		advancePosition();

		batch = renderhelper::createQuadBatch(_computedLuminance, _materials.debug, _renderer->clampSampler(), renderhelper::QuadType::Default);
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
