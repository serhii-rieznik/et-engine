/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/drawer/cubemaps.h>

namespace et
{
namespace s3d
{

CubemapProcessor::CubemapProcessor() :
	FlagsHolder()
{
	for (uint32_t level = 0; level < CubemapLevels; ++level)
	{
		for (uint32_t layer = 0; layer < 6; ++layer)
			_wholeCubemapBeginInfo.subpasses.emplace_back(layer, level);
	}

	Camera cm;
	const mat4& proj = cm.perspectiveProjection(HALF_PI, 1.0f, 1.0f, 2.0f);
	_projections = cubemapMatrixProjectionArray(proj, vec3(0.0f));
	for (mat4& m : _projections)
		m = m.inverted();
}

void CubemapProcessor::processAtmosphere()
{
	removeFlag(CubemapProcessed);
	setFlag(CubemapAtmosphere);
}

void CubemapProcessor::processEquiretangularTexture(const Texture::Pointer& tex)
{
	_sourceTextureName = tex->origin();
	_tex[CubemapType::Source] = tex;
	removeFlag(CubemapProcessed);
	removeFlag(CubemapAtmosphere);
}

void CubemapProcessor::process(RenderInterface::Pointer& renderer, DrawerOptions& options, const Light::Pointer& light)
{
	validate(renderer);

	if (!hasFlag(BRDFLookupProcessed) || options.rebuildLookupTexture)
	{
		if (_lookupGeneratorMaterial.invalid())
			_lookupGeneratorMaterial = renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::EnvironmentMap);

		_lookupPass->begin(RenderPassBeginInfo::singlePass());
		_lookupPass->nextSubpass();
		_lookupPass->pushRenderBatch(renderhelper::createFullscreenRenderBatch(renderer->checkersTexture(), _lookupGeneratorMaterial));
		_lookupPass->endSubpass();
		_lookupPass->end();
		renderer->submitRenderPass(_lookupPass);

		setFlag(BRDFLookupProcessed);
		options.rebuildLookupTexture = false;
	}

	if (!hasFlag(CubemapProcessed) || options.rebuldEnvironmentProbe)
	{	
		_downsamplePass->begin(_wholeCubemapBeginInfo);
		_downsamplePass->loadSharedVariablesFromLight(light);
		
		RenderBatch::Pointer copyBatch = renderhelper::createFullscreenRenderBatch(_tex[CubemapType::Source], 
			hasFlag(CubemapAtmosphere) ? _atmosphereMaterial : _wrapMaterial, _eqMapSampler, ResourceRange(0, 1, 0, 1));
		
		_downsamplePass->pushImageBarrier(_tex[CubemapType::Downsampled], ResourceBarrier(TextureState::ColorRenderTarget, 0, 1, 0, 6));
		for (uint32_t face = 0; face < 6; ++face)
		{
			_downsamplePass->setSharedVariable(ObjectVariable::WorldTransform, _projections[face]);
			_downsamplePass->nextSubpass();
			_downsamplePass->pushRenderBatch(copyBatch);
			_downsamplePass->endSubpass();
		}

		RenderBatch::Pointer downsampleBatch = renderhelper::createFullscreenRenderBatch(_tex[CubemapType::Downsampled], _downsampleMaterial, renderer->defaultSampler(), ResourceRange(0, 1, 0, 6));
		for (uint32_t level = 1; level < CubemapLevels; ++level)
		{
			downsampleBatch->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(level));
			downsampleBatch->material()->setTexture(MaterialTexture::BaseColor, _tex[CubemapType::Downsampled], { 0, level, 0, 6 });

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

		_downsamplePass->pushImageBarrier(_tex[CubemapType::Downsampled], ResourceBarrier(TextureState::ShaderResource, 0, CubemapLevels, 0, 6));
		_downsamplePass->end();
		
		renderer->submitRenderPass(_downsamplePass);

		Material::Pointer mtl = _specularConvolveBatch->material();
		mtl->setTexture(MaterialTexture::BaseColor, _tex[CubemapType::Downsampled]);
		_specularConvolvePass->begin(_wholeCubemapBeginInfo);
		for (uint32_t i = 0, e = static_cast<uint32_t>(_wholeCubemapBeginInfo.subpasses.size()); i < e; ++i)
		{
			uint32_t level = i / 6;
			uint32_t face = i % 6;
			vec2 sz = vector2ToFloat(_tex[CubemapType::Downsampled]->size(level));
			mtl->setVector(MaterialVariable::ExtraParameters, vec4(static_cast<float>(level), sz.x, sz.y, static_cast<float>(face)));
			_specularConvolvePass->nextSubpass();
			_specularConvolvePass->setSharedVariable(ObjectVariable::WorldTransform, _projections[face]);
			_specularConvolvePass->pushRenderBatch(_specularConvolveBatch);
			_specularConvolvePass->endSubpass();
		}
		_specularConvolvePass->end();
		renderer->submitRenderPass(_specularConvolvePass);
		
		setFlag(CubemapProcessed);
		options.rebuldEnvironmentProbe = false;
	}

	drawDebug(renderer, options);
}

void CubemapProcessor::validate(RenderInterface::Pointer& renderer)
{
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
		cubemapDesc->flags = Texture::Flags::RenderTarget;
		cubemapDesc->levelCount = CubemapLevels;
		cubemapDesc->layerCount = 6;
		cubemapDesc->size = vec2i(1 << (CubemapLevels - 1));
		if (_tex[CubemapType::Downsampled].invalid())
			_tex[CubemapType::Downsampled] = renderer->createTexture(cubemapDesc);
		if (_tex[CubemapType::Convoluted].invalid())
			_tex[CubemapType::Convoluted] = renderer->createTexture(cubemapDesc);
	}

	uint32_t passPriority = RenderPassPriority::Default + 0x100;

	if (_lookupPass.invalid())
	{
		TextureDescription::Pointer lookupDesc(PointerInit::CreateInplace);
		lookupDesc->format = TextureFormat::RGBA16F;
		lookupDesc->size = vec2i(256);
		lookupDesc->flags = Texture::Flags::RenderTarget;
		_lookup = renderer->createTexture(lookupDesc);

		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].enabled = true;
		passInfo.color[0].texture = _lookup;
		passInfo.color[0].loadOperation = FramebufferOperation::DontCare;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = false;
		passInfo.name = "generate-split-sum-approx";
		passInfo.priority = passPriority--;
		_lookupPass = renderer->allocateRenderPass(passInfo);

		passInfo.color[0].loadOperation = FramebufferOperation::Load;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = true;
		passInfo.name = "default";
		passInfo.priority = RenderPassPriority::UI - 2;
		_lookupDebugPass = renderer->allocateRenderPass(passInfo);

		Material::Pointer lookupDebugMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/textured2d-transformed.json"));
		_lookupDebugBatch = renderhelper::createFullscreenRenderBatch(_lookup, lookupDebugMaterial);
	}

	if (_downsamplePass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].enabled = true;
		passInfo.color[0].texture = _tex[CubemapType::Downsampled];
		passInfo.color[0].loadOperation = FramebufferOperation::DontCare;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = false;
		passInfo.name = "prepare-cubemap";
		passInfo.priority = passPriority--;
		_downsamplePass = renderer->allocateRenderPass(passInfo);
	}

	if (_specularConvolvePass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].enabled = true;
		passInfo.color[0].texture = _tex[CubemapType::Convoluted];
		passInfo.color[0].loadOperation = FramebufferOperation::DontCare;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = false;
		passInfo.name = "cubemap-specular-convolution";
		passInfo.priority = passPriority--;
		_specularConvolvePass = renderer->allocateRenderPass(passInfo);
		_specularConvolveBatch = renderhelper::createFullscreenRenderBatch(_tex[CubemapType::Downsampled], _processingMaterial);
	}

	if (_cubemapDebugPass.invalid())
	{
		RenderPass::ConstructionInfo passInfo;
		passInfo.color[0].enabled = true;
		passInfo.color[0].loadOperation = FramebufferOperation::Load;
		passInfo.color[0].storeOperation = FramebufferOperation::Store;
		passInfo.color[0].useDefaultRenderTarget = true;
		passInfo.name = "cubemap-visualize";
		passInfo.priority = RenderPassPriority::UI - 1;
		_cubemapDebugPass = renderer->allocateRenderPass(passInfo);
		_cubemapDebugBatch = renderhelper::createFullscreenRenderBatch(renderer->checkersTexture(), _processingMaterial);
	}
}

void CubemapProcessor::drawDebug(RenderInterface::Pointer& renderer, const DrawerOptions& options)
{
	vec2 vp = vector2ToFloat(renderer->rc()->size());

	if (options.drawEnvironmentProbe)
	{
		float dy = vp.y / static_cast<float>(CubemapLevels);
		float dx = 2.0f * dy;
		vec2 pos = vec2(0.0f, 0.0f);

		_cubemapDebugPass->begin(RenderPassBeginInfo::singlePass());
		_cubemapDebugPass->nextSubpass();
		for (uint32_t i = CubemapType::Downsampled; i < CubemapType::Count; ++i)
		{
			pos.y = 0.0f;
			_cubemapDebugBatch->material()->setTexture(MaterialTexture::BaseColor, _tex[i]);
			for (uint32_t j = 0; j < CubemapLevels; ++j)
			{
				_cubemapDebugBatch->material()->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(j));
				_cubemapDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, pos, vec2(dx, dy)));
				_cubemapDebugPass->pushRenderBatch(_cubemapDebugBatch);
				pos.y += dy;
			}
			pos.x += dx;
		}
		_cubemapDebugPass->endSubpass();
		_cubemapDebugPass->end();
		renderer->submitRenderPass(_cubemapDebugPass);
	}

	if (options.drawLookupTexture)
	{
		vec2 lookupSize = vec2(256.0f);
		_lookupDebugPass->begin(RenderPassBeginInfo::singlePass());
		_lookupDebugPass->nextSubpass();
		_lookupDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, 0.5f * (vp - lookupSize), lookupSize));
		_lookupDebugPass->pushRenderBatch(_lookupDebugBatch);
		_lookupDebugPass->endSubpass();
		_lookupDebugPass->end();
		renderer->submitRenderPass(_lookupDebugPass);
	}
}

const Texture::Pointer& CubemapProcessor::convolutedCubemap() const
{
	return _tex[CubemapType::Convoluted];
}

const Texture::Pointer& CubemapProcessor::brdfLookupTexture() const
{
	return _lookup;
}

const std::string& CubemapProcessor::sourceTextureName() const
{
	return _sourceTextureName;
}

}
}
