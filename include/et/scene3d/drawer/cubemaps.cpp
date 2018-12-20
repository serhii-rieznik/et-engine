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
	const mat4& proj = cm.perspectiveProjection(HALF_PI, 1.0f, 1.0f, 2.0f, false);
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

	_atmosphere.enableMultipleScattering = options.enableMultipleScattering;

	if ((_atmospherePrecomputed == false) || options.rebuildEnvironmentTextures)
	{
		renderer->submitPassWithRenderBatch(_atmosphere.opticalDepthPass, _atmosphere.opticalDepthBatch);

		_atmosphere.singleScatteringBatch->material()->setTexture("precomputedOpticalDepth", _atmosphere.opticalDepth);
		renderer->submitPassWithRenderBatch(_atmosphere.singleScatteringPass, _atmosphere.singleScatteringBatch);

		if (options.enableMultipleScattering)
		{
			_atmosphere.multipleScatteringBatch->material()->setTexture("precomputedOpticalDepth", _atmosphere.opticalDepth);

			Texture::Pointer sourceTexture = _atmosphere.singleScattering;
			for (uint32_t i = 0; i < ScatteringOrder; ++i)
			{
				_atmosphere.multipleScatteringBatch->material()->setTexture("precomputedInScattering", sourceTexture);

				renderer->beginRenderPass(_atmosphere.multipleScatteringPass[i], RenderPassBeginInfo::singlePass());
				{
					Texture::Pointer targetTexture = _atmosphere.multipleScatteringPass[i]->info().color[0].texture;
					_atmosphere.multipleScatteringPass[i]->pushImageBarrier(sourceTexture, ResourceBarrier(TextureState::ShaderResource));
					_atmosphere.multipleScatteringPass[i]->pushImageBarrier(targetTexture, ResourceBarrier(TextureState::ColorRenderTarget));
					_atmosphere.multipleScatteringPass[i]->addSingleRenderBatchSubpass(_atmosphere.multipleScatteringBatch);
					sourceTexture = targetTexture;
				}
				renderer->submitRenderPass(_atmosphere.multipleScatteringPass[i]);
			}

			renderer->beginRenderPass(_atmosphere.combineScatteringPass, RenderPassBeginInfo::singlePass());
			_atmosphere.combineScatteringBatch->material()->setTexture("order0", _atmosphere.singleScattering);
			_atmosphere.combineScatteringBatch->material()->setTexture("order1", _atmosphere.multipleScattering[0]);
			_atmosphere.combineScatteringBatch->material()->setTexture("order2", _atmosphere.multipleScattering[1]);
			_atmosphere.combineScatteringBatch->material()->setTexture("order3", _atmosphere.multipleScattering[2]);
			_atmosphere.combineScatteringBatch->material()->setTexture("order4", _atmosphere.multipleScattering[3]);
			_atmosphere.combineScatteringPass->pushImageBarrier(_atmosphere.singleScattering, ResourceBarrier(TextureState::ShaderResource));
			_atmosphere.combineScatteringPass->pushImageBarrier(_atmosphere.multipleScattering[0], ResourceBarrier(TextureState::ShaderResource));
			_atmosphere.combineScatteringPass->pushImageBarrier(_atmosphere.multipleScattering[1], ResourceBarrier(TextureState::ShaderResource));
			_atmosphere.combineScatteringPass->pushImageBarrier(_atmosphere.multipleScattering[2], ResourceBarrier(TextureState::ShaderResource));
			_atmosphere.combineScatteringPass->pushImageBarrier(_atmosphere.multipleScattering[3], ResourceBarrier(TextureState::ShaderResource));
			_atmosphere.combineScatteringPass->addSingleRenderBatchSubpass(_atmosphere.combineScatteringBatch);
			renderer->submitRenderPass(_atmosphere.combineScatteringPass);
		}

		_atmospherePrecomputed = true;
	}

	if (_grabHarmonicsFrame == 0)
	{
		uint8_t* ptr = _shValuesBuffer->map(0, _shValuesBuffer->size());
		memcpy(_environmentSphericalHarmonics, ptr, sizeof(_environmentSphericalHarmonics));
		_shValuesBuffer->unmap();

		uint32_t order = 0;
		for (const vec4& sh : _environmentSphericalHarmonics)
			log::info("%04u:\t%0.4f\t%0.4f\t%0.4f\t%0.4f", order++, sh.x, sh.y, sh.z, sh.w);

		_grabHarmonicsFrame = -1;
	}
	else if (_grabHarmonicsFrame > 0)
	{
		--_grabHarmonicsFrame;
	}

	if (!hasFlag(BRDFLookupProcessed) || options.rebuildEnvironmentTextures)
	{
		if (_lookupGeneratorMaterial.invalid())
			_lookupGeneratorMaterial = renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::EnvironmentMap);

		renderer->beginRenderPass(_lookupPass, RenderPassBeginInfo::singlePass());
		_lookupPass->nextSubpass();
		_lookupPass->pushRenderBatch(renderhelper::createQuadBatch(_lookupGeneratorMaterial));
		_lookupPass->endSubpass();
		renderer->submitRenderPass(_lookupPass);

		setFlag(BRDFLookupProcessed);
	}

	if (!hasFlag(CubemapProcessed) || options.rebuildEnvironmentTextures)
	{
		renderer->beginRenderPass(_downsamplePass, _wholeCubemapBeginInfo);
		_downsamplePass->loadSharedVariablesFromLight(light);

		RenderBatch::Pointer copyBatch;
		if (hasFlag(CubemapAtmosphere))
		{
			copyBatch = renderhelper::createQuadBatch(_atmosphereMaterial);
			copyBatch->material()->setTexture("precomputedOpticalDepth", _atmosphere.opticalDepth);
			copyBatch->material()->setTexture("precomputedInScattering", _atmosphere.singleScattering);
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

		if (_grabHarmonicsFrame == -1)
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

		setFlag(CubemapProcessed);
	}

	drawDebug(renderer, options);

	options.rebuildEnvironmentTextures = false;
}

void CubemapProcessor::validate(RenderInterface::Pointer& renderer) {
	if (_processingMaterial.invalid())
		_processingMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap.json"));

	if (_cubemapDebugMaterial.invalid())
		_cubemapDebugMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap-debug.json"));

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
		passInfo.priority = RenderPassPriority::Preprocess;
		_lookupPass = renderer->allocateRenderPass(passInfo);
	}

	if (_lookupDebugBatch.invalid())
	{
		Material::Pointer debugMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed.json"));
		_lookupDebugBatch = renderhelper::createQuadBatch(MaterialTexture::Input, _lookup, debugMaterial, renderhelper::QuadType::Default);
	}

	uint32_t passPriority = RenderPassPriority::Preprocess - 1;
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

	if (_cubemapDebugPass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].loadOperation = FramebufferOperation::Load;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::DefaultBuffer;
		passInfo.name = "default";
		passInfo.priority = RenderPassPriority::UI + 1;
		_cubemapDebugPass = renderer->allocateRenderPass(passInfo);
		_cubemapDebugBatch = renderhelper::createQuadBatch(_cubemapDebugMaterial, renderhelper::QuadType::Default);
	}

	if (_atmosphere.opticalDepthPass.invalid())
	{
		TextureDescription::Pointer txDesc(PointerInit::CreateInplace);
		txDesc->format = TextureFormat::RGBA32F;
		txDesc->size = vec2i(256, 256);
		txDesc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget;
		_atmosphere.opticalDepth = renderer->createTexture(txDesc);

		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].loadOperation = FramebufferOperation::Clear;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::Texture;
		passInfo.color[0].texture = _atmosphere.opticalDepth;
		passInfo.name = "precompute-optical-depth";
		passInfo.priority = RenderPassPriority::Preprocess;

		_atmosphere.opticalDepthPass = renderer->allocateRenderPass(passInfo);
		_atmosphere.opticalDepthBatch = renderhelper::createQuadBatch(_atmosphereMaterial);

		Material::Pointer debugMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed.json"));
		_atmosphere.opticalDepthBatchDebug = renderhelper::createQuadBatch(MaterialTexture::Input, _atmosphere.opticalDepth, debugMaterial, renderhelper::QuadType::Default);
	}

	if (_atmosphere.singleScattering.invalid())
	{
		TextureDescription::Pointer txDesc(PointerInit::CreateInplace);
		txDesc->format = TextureFormat::RGBA32F;
		txDesc->size = vec2i(128, 128);
		txDesc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget | Texture::Flags::CopyDestination;
		_atmosphere.singleScattering = renderer->createTexture(txDesc);

		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].loadOperation = FramebufferOperation::Clear;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::Texture;
		passInfo.color[0].texture = _atmosphere.singleScattering;
		passInfo.name = "precompute-single-scattering";
		passInfo.priority = RenderPassPriority::Preprocess;

		_atmosphere.singleScatteringPass = renderer->allocateRenderPass(passInfo);
		_atmosphere.singleScatteringBatch = renderhelper::createQuadBatch(_atmosphereMaterial);

		Material::Pointer debugMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed.json"));
		_atmosphere.inScatteringBatchDebug = renderhelper::createQuadBatch(MaterialTexture::Input, _atmosphere.singleScattering, debugMaterial, renderhelper::QuadType::Default);
	}

	if (_atmosphere.multipleScatteringInitialized == false)
	{
		TextureDescription::Pointer txDesc(PointerInit::CreateInplace);
		txDesc->format = TextureFormat::RGBA32F;
		txDesc->size = vec2i(128, 128);
		txDesc->flags = Texture::Flags::ShaderResource | Texture::Flags::RenderTarget | Texture::Flags::CopySource;
		_atmosphere.finalScattering = renderer->createTexture(txDesc);

		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].loadOperation = FramebufferOperation::Clear;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].targetClass = RenderTarget::Class::Texture;
		passInfo.name = "precompute-multiple-scattering";
		passInfo.priority = RenderPassPriority::Preprocess;

		for (uint32_t i = 0; i < ScatteringOrder; ++i)
		{
			_atmosphere.multipleScattering[i] = renderer->createTexture(txDesc);
			passInfo.color[0].texture = _atmosphere.multipleScattering[i];
			_atmosphere.multipleScatteringPass[i] = renderer->allocateRenderPass(passInfo);
		}

		passInfo.name = "combine-in-scattering";
		passInfo.color[0].texture = _atmosphere.finalScattering;
		_atmosphere.combineScatteringPass = renderer->allocateRenderPass(passInfo);

		_atmosphere.multipleScatteringBatch = renderhelper::createQuadBatch(_atmosphereMaterial);
		_atmosphere.combineScatteringBatch = renderhelper::createQuadBatch(_atmosphereMaterial);
		_atmosphere.multipleScatteringInitialized = true;
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

	if (options.drawCubemapsDebug)
	{
		float cubemapsCount = static_cast<float>(CubemapType::Count - CubemapType::Downsampled);
		float dy = std::floor(vp.y / static_cast<float>(CubemapLevels));
		float dx = 2.0f * dy;
		// float ds = std::max(dx, dy);
		float xGap = 0.1f * dx;
		float yGap = 0.1f * dy;
		vec2 pos = vec2(0.5f * (vp.x - dx * cubemapsCount - xGap * (cubemapsCount - 1.0f)), 0.0f);

		renderer->beginRenderPass(_cubemapDebugPass, RenderPassBeginInfo::singlePass());
		_cubemapDebugPass->nextSubpass();
		/*
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
			if (i + 1 < CubemapType::Count)
				pos.x += dx + xGap;
		}
		pos.y += yGap;

		pos.y += dy;

		_cubemapDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(ds, ds)));
		_cubemapDebugPass->pushRenderBatch(_lookupDebugBatch);
		pos.y += ds;
		*/

		{
			_cubemapDebugPass->setSharedVariable(ObjectVariable::EnvironmentSphericalHarmonics, _environmentSphericalHarmonics, 9);
			_cubemapDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
			_cubemapDebugPass->pushRenderBatch(_shDebugBatch);
			pos.y += dy + yGap;
		}
		
		{
			vec2 inScatteringSize = vector2ToFloat(_atmosphere.singleScattering->size(0));
			inScatteringSize.x = std::min(inScatteringSize.x, 2.0f / 3.0f * vp.x);
			inScatteringSize.y *= std::min(inScatteringSize.x, 2.0f / 3.0f * vp.x) / inScatteringSize.x;

			_atmosphere.inScatteringBatchDebug->material()->setTexture(MaterialTexture::Input, _atmosphere.singleScattering);
			_cubemapDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, inScatteringSize));
			_cubemapDebugPass->pushRenderBatch(_atmosphere.inScatteringBatchDebug);
			pos.y += inScatteringSize.y + yGap;
		}

		float originalPosX = pos.x;
		float yStep = 0;
		for (uint32_t i = 0; i < ScatteringOrder; ++i)
		{
			vec2 inScatteringSize = vector2ToFloat(_atmosphere.multipleScattering[i]->size(0));
			inScatteringSize.x = std::min(inScatteringSize.x, 2.0f / 3.0f * vp.x);
			inScatteringSize.y *= std::min(inScatteringSize.x, 2.0f / 3.0f * vp.x) / inScatteringSize.x;

			_atmosphere.inScatteringBatchDebug->material()->setTexture(MaterialTexture::Input, _atmosphere.multipleScattering[i]);
			_cubemapDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, inScatteringSize));
			_cubemapDebugPass->pushRenderBatch(_atmosphere.inScatteringBatchDebug);
			yStep = std::max(yStep, inScatteringSize.y);
			pos.x += inScatteringSize.x + xGap;
		}
		{
			vec2 inScatteringSize = vector2ToFloat(_atmosphere.finalScattering->size(0));
			inScatteringSize.x = std::min(inScatteringSize.x, 2.0f / 3.0f * vp.x);
			inScatteringSize.y *= std::min(inScatteringSize.x, 2.0f / 3.0f * vp.x) / inScatteringSize.x;

			_atmosphere.inScatteringBatchDebug->material()->setTexture(MaterialTexture::Input, _atmosphere.finalScattering);
			_cubemapDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, inScatteringSize));
			_cubemapDebugPass->pushRenderBatch(_atmosphere.inScatteringBatchDebug);
			yStep = std::max(yStep, inScatteringSize.y);
		}
		pos.x = originalPosX;
		pos.y += yStep + yGap;

		{
			vec2 transmittanceSize = vector2ToFloat(_atmosphere.opticalDepth->size(0));
			transmittanceSize.x = std::min(transmittanceSize.x, 2.0f / 3.0f * vp.x);
			transmittanceSize.y *= std::min(transmittanceSize.x, 2.0f / 3.0f * vp.x) / transmittanceSize.x;

			_cubemapDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, transmittanceSize));
			_cubemapDebugPass->pushRenderBatch(_atmosphere.opticalDepthBatchDebug);
		}
		_cubemapDebugPass->endSubpass();
		renderer->submitRenderPass(_cubemapDebugPass);
	}
}

}
}
