/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/drawer/common.h>

namespace et {
namespace s3d {

class CubemapProcessor : public Object, public FlagsHolder
{
public:
	ET_DECLARE_POINTER(CubemapProcessor);

public:
	CubemapProcessor();

	const std::string& sourceTextureName() const;
	const Texture::Pointer& convolvedDiffuseCubemap() const;
	const Texture::Pointer& convolvedSpecularCubemap() const;
	const Texture::Pointer& brdfLookupTexture() const;
	const Texture::Pointer& precomputedOpticalDepthTexture() const;
	const Texture::Pointer& precomputedInScatteringTexture() const;
	const vec4* environmentSphericalHarmonics() const;

	void processAtmosphere();
	void processEquiretangularTexture(const Texture::Pointer&);
	void process(RenderInterface::Pointer&, DrawerOptions&, const Light::Pointer&);

private:
	void validate(RenderInterface::Pointer& renderer);
	void drawDebug(RenderInterface::Pointer&, const DrawerOptions&);

private:
	enum CubemapType : uint32_t
	{
		Source,
		Downsampled,
		Specular,
		Diffuse,
		Count
	};

	enum : uint32_t
	{
		/*
		 * Flags
		 */
		CubemapProcessed = 1 << 0,
		BRDFLookupProcessed = 1 << 1,
		CubemapAtmosphere = 1 << 2,

		/*
		 * Constants
		 */
		CubemapLevels = 8
	};

private:
	Texture::Pointer _lookup;
	Texture::Pointer _tex[CubemapType::Count];
	Sampler::Pointer _eqMapSampler;
	Material::Pointer _wrapMaterial;
	Material::Pointer _atmosphereMaterial;
	Material::Pointer _processingMaterial;
	Material::Pointer _cubemapDebugMaterial;
	Material::Pointer _shMaterial;
	Material::Pointer _downsampleMaterial;
	Material::Pointer _lookupGeneratorMaterial;

	Texture::Pointer _shValues;
	Buffer::Pointer _shValuesBuffer;
	Compute::Pointer _shConvolute;

	struct
	{
		RenderPass::Pointer opticalDepthPass;
		RenderBatch::Pointer opticalDepthBatch;
		RenderBatch::Pointer opticalDepthBatchDebug;
		RenderBatch::Pointer inScatteringBatchDebug;
		Texture::Pointer opticalDepth;

		RenderPass::Pointer inScatteringPass;
		RenderBatch::Pointer inScatteringBatch;
		Texture::Pointer inScattering;
	} _atmosphere;

	RenderPass::Pointer _lookupPass;
	RenderPass::Pointer _downsamplePass;
	RenderPass::Pointer _cubemapDebugPass;
	RenderPass::Pointer _diffuseConvolvePass;
	RenderPass::Pointer _specularConvolvePass;
	RenderBatch::Pointer _lookupDebugBatch;

	RenderBatch::Pointer _cubemapDebugBatch;
	RenderBatch::Pointer _shDebugBatch;
	RenderBatch::Pointer _diffuseConvolveBatch;
	RenderBatch::Pointer _specularConvolveBatch;
	CubemapProjectionMatrixArray _projections;
	RenderPassBeginInfo _oneLevelCubemapBeginInfo;
	RenderPassBeginInfo _wholeCubemapBeginInfo;

	vec4 _environmentSphericalHarmonics[9]{};
	std::string _sourceTextureName;
	int32_t _grabHarmonicsFrame = -1;
	bool _atmospherePrecomputed = false;
};

inline const Texture::Pointer& CubemapProcessor::convolvedDiffuseCubemap() const {
	return _tex[CubemapType::Diffuse];
}

inline const Texture::Pointer& CubemapProcessor::convolvedSpecularCubemap() const {
	return _tex[CubemapType::Specular];
}

inline const Texture::Pointer& CubemapProcessor::brdfLookupTexture() const {
	return _lookup;
}

inline const Texture::Pointer& CubemapProcessor::precomputedOpticalDepthTexture() const {
	return _atmosphere.opticalDepth;
}

inline const Texture::Pointer& CubemapProcessor::precomputedInScatteringTexture() const {
	return _atmosphere.inScattering;
}

inline const std::string& CubemapProcessor::sourceTextureName() const {
	return _sourceTextureName;
}

inline const vec4* CubemapProcessor::environmentSphericalHarmonics() const {
	return _environmentSphericalHarmonics;
}

}
}
