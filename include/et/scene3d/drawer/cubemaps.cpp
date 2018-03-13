/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/drawer/cubemaps.h>

namespace et {
namespace s3d {

CubemapProcessor::CubemapProcessor() :
	FlagsHolder() {

	for (uint32_t level = 0; level < CubemapLevels; ++level)
	{
		for (uint32_t layer = 0; layer < 6; ++layer)
			_wholeCubemapBeginInfo.subpasses.emplace_back(layer, level);
	}

	for (uint32_t layer = 0; layer < 6; ++layer)
		_oneLevelCubemapBeginInfo.subpasses.emplace_back(layer, 0);

	Camera cm;
	const mat4& proj = cm.perspectiveProjection(HALF_PI, 1.0f, 1.0f, 2.0f);
	_projections = cubemapMatrixProjectionArray(proj, vec3(0.0f));
	for (mat4& m : _projections)
		m = m.inverted();
}

void CubemapProcessor::processAtmosphere() {
	removeFlag(CubemapProcessed);
	setFlag(CubemapAtmosphere);
}

void CubemapProcessor::processEquiretangularTexture(const Texture::Pointer& tex) {
	_sourceTextureName = tex->origin();
	_tex[CubemapType::Source] = tex;
	removeFlag(CubemapProcessed);
	removeFlag(CubemapAtmosphere);
}

void CubemapProcessor::process(RenderInterface::Pointer& renderer, DrawerOptions& options, const Light::Pointer& light) {
	validate(renderer);

	if (_grabHarmonicsFrame == 0)
	{
		uint8_t* ptr = _shValuesBuffer->map(0, _shValuesBuffer->size());
		memcpy(_environmentSphericalHarmonics, ptr, sizeof(_environmentSphericalHarmonics));
		_shValuesBuffer->unmap();
		_grabHarmonicsFrame = -1;
	}
	else if (_grabHarmonicsFrame > 0)
	{
		--_grabHarmonicsFrame;
	}

	if (!hasFlag(BRDFLookupProcessed) || options.rebuildLookupTexture)
	{
		if (_lookupGeneratorMaterial.invalid())
			_lookupGeneratorMaterial = renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::EnvironmentMap);

		renderer->beginRenderPass(_lookupPass, RenderPassBeginInfo::singlePass());
		_lookupPass->nextSubpass();
		_lookupPass->pushRenderBatch(renderhelper::createQuadBatch(_lookupGeneratorMaterial));
		_lookupPass->endSubpass();
		renderer->submitRenderPass(_lookupPass);

		setFlag(BRDFLookupProcessed);
		options.rebuildLookupTexture = false;
	}

	if (!hasFlag(CubemapProcessed) || options.rebuldEnvironmentProbe)
	{
		renderer->beginRenderPass(_downsamplePass, _wholeCubemapBeginInfo);
		_downsamplePass->loadSharedVariablesFromLight(light);

		RenderBatch::Pointer copyBatch;
		if (hasFlag(CubemapAtmosphere))
		{
			copyBatch = renderhelper::createQuadBatch(_atmosphereMaterial);
		}
		else
		{
			copyBatch = renderhelper::createQuadBatch(_wrapMaterial);
			copyBatch->material()->setTexture(MaterialTexture::Input, _tex[CubemapType::Source], ResourceRange(0, 1, 0, 1));
			copyBatch->material()->setSampler("eqMapSampler", _eqMapSampler);
		}

		_downsamplePass->pushImageBarrier(_tex[CubemapType::Downsampled], ResourceBarrier(TextureState::ColorRenderTarget, 0, 1, 0, 6));
		for (uint32_t face = 0; face < 6; ++face)
		{
			_downsamplePass->setSharedVariable(ObjectVariable::WorldTransform, _projections[face]);
			_downsamplePass->nextSubpass();
			_downsamplePass->pushRenderBatch(copyBatch);
			_downsamplePass->endSubpass();
		}

		RenderBatch::Pointer downsampleBatch = renderhelper::createQuadBatch(_downsampleMaterial);
		for (uint32_t level = 1; level < CubemapLevels; ++level)
		{
			downsampleBatch->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(level));
			downsampleBatch->material()->setTexture(MaterialTexture::Input, _tex[CubemapType::Downsampled], { 0, level, 0, 6 });

			_downsamplePass->pushImageBarrier(_tex[CubemapType::Downsampled],
				ResourceBarrier(TextureState::ColorRenderTarget, level, 1, 0, 6));

			_downsamplePass->pushImageBarrier(_tex[CubemapType::Downsampled],
				ResourceBarrier(TextureState::ShaderResource, level - 1, 1, 0, 6));

			for (uint32_t face = 0; face < 6; ++face)
			{
				_downsamplePass->setSharedVariable(ObjectVariable::WorldTransform, _projections[face]);
				_downsamplePass->nextSubpass();
				_downsamplePass->pushRenderBatch(downsampleBatch);
				_downsamplePass->endSubpass();
			}
		}

		_shConvolute->material()->setTexture(MaterialTexture::Input, _tex[CubemapType::Downsampled]);
		_shConvolute->material()->setImage(MaterialTexture::OutputImage, _shValues);

		_downsamplePass->pushImageBarrier(_tex[CubemapType::Downsampled], ResourceBarrier(TextureState::ShaderResource, 0, CubemapLevels, 0, 6));
		_downsamplePass->pushImageBarrier(_shValues, ResourceBarrier(TextureState::Storage));
		_downsamplePass->dispatchCompute(_shConvolute, vec3i(1, 1, 1));
		_downsamplePass->pushImageBarrier(_shValues, ResourceBarrier(TextureState::CopySource));
		_downsamplePass->copyImageToBuffer(_shValues, _shValuesBuffer, CopyDescriptor(vec3i(_shValues->size(0), 1)));
		renderer->submitRenderPass(_downsamplePass);

		_grabHarmonicsFrame = RendererFrameCount;

		//*
		_specularConvolveBatch->material()->setTexture(MaterialTexture::Input, _tex[CubemapType::Downsampled]);
		renderer->beginRenderPass(_specularConvolvePass, _wholeCubemapBeginInfo);
		for (uint32_t i = 0, e = static_cast<uint32_t>(_wholeCubemapBeginInfo.subpasses.size()); i < e; ++i)
		{
			uint32_t level = i / 6;
			uint32_t face = i % 6;
			vec2 sz = vector2ToFloat(_tex[CubemapType::Downsampled]->size(level));
			_specularConvolveBatch->material()->setVector(MaterialVariable::ExtraParameters, vec4(static_cast<float>(level), sz.x, sz.y, static_cast<float>(face)));
			_specularConvolvePass->nextSubpass();
			_specularConvolvePass->setSharedVariable(ObjectVariable::WorldTransform, _projections[face]);
			_specularConvolvePass->pushRenderBatch(_specularConvolveBatch);
			_specularConvolvePass->endSubpass();
		}
		renderer->submitRenderPass(_specularConvolvePass);
		// */

		/*
		_diffuseConvolvePass->begin(_oneLevelCubemapBeginInfo);
		_diffuseConvolveBatch->material()->setTexture(MaterialTexture::Input, _tex[CubemapType::Downsampled]);
		for (uint32_t i = 0, e = 6; i < e; ++i)
		{
			uint32_t face = i % 6;
			vec2 sz = vector2ToFloat(_tex[CubemapType::Downsampled]->size(0));
			_diffuseConvolveBatch->material()->setVector(MaterialVariable::ExtraParameters, vec4(0.0f, sz.x, sz.y, static_cast<float>(face)));
			_diffuseConvolvePass->nextSubpass();
			_diffuseConvolvePass->setSharedVariable(ObjectVariable::WorldTransform, _projections[face]);
			_diffuseConvolvePass->pushRenderBatch(_specularConvolveBatch);
			_diffuseConvolvePass->endSubpass();
		}
		_diffuseConvolvePass->end();
		renderer->submitRenderPass(_diffuseConvolvePass);
		// */

		setFlag(CubemapProcessed);
		options.rebuldEnvironmentProbe = false;
	}

	drawDebug(renderer, options);
}

void CubemapProcessor::validate(RenderInterface::Pointer& renderer) {
	if (_processingMaterial.invalid())
		_processingMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap.json"));

	if (_wrapMaterial.invalid())
		_wrapMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap-wrap.json"));

	if (_atmosphereMaterial.invalid())
		_atmosphereMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap-atmosphere.json"));

	if (_downsampleMaterial.invalid())
		_downsampleMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap-downsample.json"));

	if (_eqMapSampler.invalid())
	{
		Sampler::Description desc;
		desc.wrapU = TextureWrap::Repeat;
		desc.wrapV = TextureWrap::ClampToEdge;
		_eqMapSampler = renderer->createSampler(desc);
	}

	{
		TextureDescription::Pointer cubemapDesc(PointerInit::CreateInplace);
		cubemapDesc->format = TextureFormat::RGBA16F;
		cubemapDesc->target = TextureTarget::Texture_Cube;
		cubemapDesc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget;
		cubemapDesc->levelCount = CubemapLevels;
		cubemapDesc->layerCount = 6;
		cubemapDesc->size = vec2i(1 << (CubemapLevels - 1));

		if (_tex[CubemapType::Downsampled].invalid())
			_tex[CubemapType::Downsampled] = renderer->createTexture(cubemapDesc);

		if (_tex[CubemapType::Specular].invalid())
			_tex[CubemapType::Specular] = renderer->createTexture(cubemapDesc);

		if (_tex[CubemapType::Diffuse].invalid())
		{
			cubemapDesc->levelCount = 1;
			cubemapDesc->size = vec2i(32);
			_tex[CubemapType::Diffuse] = renderer->createTexture(cubemapDesc);
		}
	}

	uint32_t passPriority = RenderPassPriority::Default + 0x100;

	if (_lookupPass.invalid())
	{
		TextureDescription::Pointer lookupDesc(PointerInit::CreateInplace);
		lookupDesc->format = TextureFormat::RGBA16F;
		lookupDesc->size = vec2i(256);
		lookupDesc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget;
		_lookup = renderer->createTexture(lookupDesc);

		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].texture = _lookup;
		passInfo.color[0].loadOperation = FramebufferOperation::DontCare;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::Texture;
		passInfo.name = "generate-split-sum-approx";
		passInfo.priority = passPriority--;
		_lookupPass = renderer->allocateRenderPass(passInfo);

		passInfo.color[0].loadOperation = FramebufferOperation::Load;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::DefaultBuffer;
		passInfo.name = "default";
		passInfo.priority = RenderPassPriority::UI - 2;
		_lookupDebugPass = renderer->allocateRenderPass(passInfo);

		Material::Pointer lookupDebugMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed.json"));
		_lookupDebugBatch = renderhelper::createQuadBatch(MaterialTexture::Input, _lookup, lookupDebugMaterial, renderhelper::QuadType::Default);
	}

	if (_downsamplePass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].texture = _tex[CubemapType::Downsampled];
		passInfo.color[0].loadOperation = FramebufferOperation::DontCare;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::Texture;
		passInfo.name = "prepare-cubemap";
		passInfo.priority = passPriority--;
		_downsamplePass = renderer->allocateRenderPass(passInfo);
	}

	if (_specularConvolvePass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].texture = _tex[CubemapType::Specular];
		passInfo.color[0].loadOperation = FramebufferOperation::DontCare;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::Texture;
		passInfo.name = "cubemap-specular-convolution";
		passInfo.priority = passPriority--;
		_specularConvolvePass = renderer->allocateRenderPass(passInfo);
		_specularConvolveBatch = renderhelper::createQuadBatch(MaterialTexture::Input, _tex[CubemapType::Downsampled], _processingMaterial);
	}

	if (_diffuseConvolvePass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].texture = _tex[CubemapType::Diffuse];
		passInfo.color[0].loadOperation = FramebufferOperation::DontCare;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::Texture;
		passInfo.name = "cubemap-diffuse-convolution";
		passInfo.priority = passPriority--;
		_diffuseConvolvePass = renderer->allocateRenderPass(passInfo);
		_diffuseConvolveBatch = renderhelper::createQuadBatch(_processingMaterial);
		_diffuseConvolveBatch->material()->setTexture(MaterialTexture::Input, _tex[CubemapType::Downsampled]);
	}

	if (_cubemapDebugPass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].loadOperation = FramebufferOperation::Load;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::DefaultBuffer;
		passInfo.name = "cubemap-visualize";
		passInfo.priority = RenderPassPriority::UI + 1;
		_cubemapDebugPass = renderer->allocateRenderPass(passInfo);
		_cubemapDebugBatch = renderhelper::createQuadBatch(_processingMaterial, renderhelper::QuadType::Default);
	}

	if (_shDebugBatch.invalid())
	{
		_shMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/spherical-harmonics-debug.json"));
		_shDebugBatch = renderhelper::createQuadBatch(_shMaterial, renderhelper::QuadType::Default);
	}

	if (_shValues.invalid())
	{
		TextureDescription::Pointer desc(PointerInit::CreateInplace);
		desc->size = vec2i(9, 1);
		desc->flags = Texture::Flags::Storage | Texture::Flags::CopySource;
		desc->format = TextureFormat::RGBA32F;
		_shValues = renderer->createTexture(desc);

		Buffer::Description bufferDesc;
		bufferDesc.usage = Buffer::Usage::Staging;
		bufferDesc.size = sizeof(vec4) * 9;
		bufferDesc.location = Buffer::Location::Host;
		_shValuesBuffer = renderer->createBuffer("shValues", bufferDesc);
	}

	if (_shConvolute.invalid())
	{
		Material::Pointer shConvoluteMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/compute/spherical-harmonics.json"));
		_shConvolute = renderer->createCompute(shConvoluteMaterial);
	}
}

void CubemapProcessor::drawDebug(RenderInterface::Pointer& renderer, const DrawerOptions& options) {
	vec2 vp = vector2ToFloat(renderer->contextSize());

	if (options.drawEnvironmentProbe)
	{
		float cubemapsCount = static_cast<float>(CubemapType::Count - CubemapType::Downsampled);
		float dy = std::floor(vp.y / static_cast<float>(CubemapLevels));
		float dx = 2.0f * dy;
		float xGap = 0.1f * dx;
		vec2 pos = vec2(0.5f * (vp.x - dx * cubemapsCount - xGap * (cubemapsCount - 1.0f)), 0.0f);

		renderer->beginRenderPass(_cubemapDebugPass, RenderPassBeginInfo::singlePass());
		_cubemapDebugPass->nextSubpass();
		for (uint32_t i = CubemapType::Downsampled; i < CubemapType::Count; ++i)
		{
			pos.y = 0.0f;
			_cubemapDebugBatch->material()->setTexture(MaterialTexture::Input, _tex[i]);
			for (uint32_t j = 0; j < _tex[i]->description().levelCount; ++j)
			{
				_cubemapDebugBatch->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(j));
				_cubemapDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
				_cubemapDebugPass->pushRenderBatch(_cubemapDebugBatch);
				pos.y += dy;
			}
			pos.x += dx + xGap;
		}
		pos.x -= dx + xGap;

		_cubemapDebugPass->setSharedVariable(ObjectVariable::EnvironmentSphericalHarmonics, _environmentSphericalHarmonics, 9);
		_cubemapDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
		_cubemapDebugPass->pushRenderBatch(_shDebugBatch);

		_cubemapDebugPass->endSubpass();
		renderer->submitRenderPass(_cubemapDebugPass);
	}

	if (options.drawLookupTexture)
	{
		vec2 lookupSize = vec2(256.0f);
		renderer->beginRenderPass(_lookupDebugPass, RenderPassBeginInfo::singlePass());
		_lookupDebugPass->nextSubpass();
		_lookupDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, 0.5f * (vp - lookupSize), lookupSize));
		_lookupDebugPass->pushRenderBatch(_lookupDebugBatch);
		_lookupDebugPass->endSubpass();
		renderer->submitRenderPass(_lookupDebugPass);
	}
}

const Texture::Pointer& CubemapProcessor::convolvedDiffuseCubemap() const {
	return _tex[CubemapType::Diffuse];
}

const Texture::Pointer& CubemapProcessor::convolvedSpecularCubemap() const {
	return _tex[CubemapType::Specular];
}

const Texture::Pointer& CubemapProcessor::brdfLookupTexture() const {
	return _lookup;
}

const std::string& CubemapProcessor::sourceTextureName() const {
	return _sourceTextureName;
}

const vec4* CubemapProcessor::environmentSphericalHarmonics() const {
	return _environmentSphericalHarmonics;
}

}
}
