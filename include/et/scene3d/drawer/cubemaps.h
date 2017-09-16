/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/drawer/common.h>

namespace et
{
namespace s3d
{

class CubemapProcessor : public Shared, public FlagsHolder
{
public:
	ET_DECLARE_POINTER(CubemapProcessor);

public:
	CubemapProcessor();

	const std::string& sourceTextureName() const;
	const Texture::Pointer& convolutedCubemap() const;
	const Texture::Pointer& brdfLookupTexture() const;

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
		Convoluted,
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
		CubemapLevels = 9
	};

private:
	Texture::Pointer _lookup;
	Texture::Pointer _tex[CubemapType::Count];
	Sampler::Pointer _eqMapSampler;
	Material::Pointer _wrapMaterial;
	Material::Pointer _atmosphereMaterial;
	Material::Pointer _processingMaterial;
	Material::Pointer _downsampleMaterial;
	Material::Pointer _lookupGeneratorMaterial;

	RenderPass::Pointer _lookupPass;
	RenderPass::Pointer _lookupDebugPass;
	RenderPass::Pointer _downsamplePass;
	RenderPass::Pointer _cubemapDebugPass;
	RenderPass::Pointer _specularConvolvePass;
	RenderBatch::Pointer _lookupDebugBatch;
	RenderBatch::Pointer _cubemapDebugBatch;
	RenderBatch::Pointer _specularConvolveBatch;
	CubemapProjectionMatrixArray _projections;
	RenderPassBeginInfo _wholeCubemapBeginInfo;
	std::string _sourceTextureName;
};

}
}
