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

void CubemapProcessor::processEquiretangularTexture(const Texture::Pointer& tex)
{
	_sourceTextureName = tex->origin();
	_tex[CubemapType::Source] = tex;
	removeFlag(CubemapProcessed);
}

void CubemapProcessor::process(RenderInterface::Pointer& renderer, DrawerOptions& options, const Light::Pointer& light)
{
	validate(renderer);

	if (!hasFlag(BRDFLookupProcessed) || options.rebuildLookupTexture)
	{
		if (_lookupGeneratorMaterial.invalid())
			_lookupGeneratorMaterial = renderer->sharedMaterialLibrary().loadDefaultMaterial(DefaultMaterial::EnvironmentMap);

		_lookupPass->begin(RenderPassBeginInfo::singlePass);
		_lookupPass->pushRenderBatch(renderhelper::createFullscreenRenderBatch(renderer->checkersTexture(), _lookupGeneratorMaterial));
		_lookupPass->end();
		renderer->submitRenderPass(_lookupPass);

		setFlag(BRDFLookupProcessed);
		options.rebuildLookupTexture = false;
	}

	if (!hasFlag(CubemapProcessed) || options.rebuldEnvironmentProbe)
	{
		/*
		* Downsampling convolution
		*/
		_processingMaterial->setTexture(MaterialTexture::Environment, _tex[CubemapType::Source]);
		_processingMaterial->setSampler(MaterialTexture::BaseColor, _eqMapSampler);

		_downsampleMaterial->setTexture(MaterialTexture::BaseColor, _tex[CubemapType::Downsampled]);

		_downsampleBatch->setMaterial(_processingMaterial->instance());
		_processingMaterial->releaseInstances();

		_downsamplePass->begin(_wholeCubemapBeginInfo);
		_downsamplePass->loadSharedVariablesFromLight(light);
		for (uint32_t i = 0, e = static_cast<uint32_t>(_wholeCubemapBeginInfo.subpasses.size()); i < e; ++i)
		{
			uint32_t level = i / 6;
			uint32_t face = i % 6;
			_downsampleMaterial->setFloat(MaterialVariable::ExtraParameters, static_cast<float>(level));
			_downsamplePass->setSharedVariable(ObjectVariable::WorldTransform, _projections[face]);
			_downsamplePass->pushRenderBatch(_downsampleBatch);
			_downsamplePass->nextSubpass();

			if (i == 5)
			{
				_downsampleBatch->setMaterial(_downsampleMaterial->instance());
				_downsampleMaterial->releaseInstances();
			}
		}
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
			_specularConvolvePass->setSharedVariable(ObjectVariable::WorldTransform, _projections[face]);
			_specularConvolvePass->pushRenderBatch(_specularConvolveBatch);
			_specularConvolvePass->nextSubpass();
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
	{
		_processingMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap.json"));
	}

	if (_eqMapSampler.invalid())
	{
		Sampler::Description desc;
		desc.wrapU = TextureWrap::Repeat;
		desc.wrapV = TextureWrap::ClampToEdge;
		_eqMapSampler = renderer->createSampler(desc);
	}

	{
		TextureDescription::Pointer cubemapDesc(PointerInit::CreateInplace);
		cubemapDesc->format = TextureFormat::RGBA32F;
		cubemapDesc->target = TextureTarget::Texture_Cube;
		cubemapDesc->flags = Texture::Flags::RenderTarget;
		cubemapDesc->levelCount = CubemapLevels;
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
		lookupDesc->format = TextureFormat::RGBA32F;
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
		passInfo.name = "forward";
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
		passInfo.name = "eq-to-cubemap";
		passInfo.priority = passPriority--;
		_downsamplePass = renderer->allocateRenderPass(passInfo);
		_downsampleBatch = renderhelper::createFullscreenRenderBatch(_tex[CubemapType::Source], _processingMaterial);

		_downsampleMaterial = renderer->sharedMaterialLibrary().loadMaterial(application().resolveFileName("engine_data/materials/cubemap-downsample.json"));
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

		_cubemapDebugPass->begin(RenderPassBeginInfo::singlePass);
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

		_cubemapDebugPass->end();
		renderer->submitRenderPass(_cubemapDebugPass);
	}

	if (options.drawLookupTexture)
	{
		vec2 lookupSize = vec2(256.0f);
		_lookupDebugPass->begin(RenderPassBeginInfo::singlePass);
		_lookupDebugPass->setSharedVariable(ObjectVariable::WorldTransform, fullscreenBatchTransform(vp, 0.5f * (vp - lookupSize), lookupSize));
		_lookupDebugPass->pushRenderBatch(_lookupDebugBatch);
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
