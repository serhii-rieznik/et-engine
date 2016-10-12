/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/texture.h>
#include <et/rendering/interface/sampler.h>

namespace et
{

enum class MaterialTexture : uint32_t
{
	Albedo,
	Reflectance,
	Roughness,
	Emissive,
	Opacity,
	Normal,

	Max
};

enum class MaterialParameter : uint32_t
{
	AmbientColor,
	DiffuseColor,
	SpecularColor,
	EmissiveColor,
	Roughness,
	Opacity,
	NormalTextureScale,
	
	Max
};

enum : uint32_t
{
	MaterialTexture_Max = static_cast<uint32_t>(MaterialTexture::Max),
	MaterialParameter_Max = static_cast<uint32_t>(MaterialParameter::Max),
};

class Material : public Object
{
public:
	ET_DECLARE_POINTER(Material);
};

class MaterialInstance : public Object
{
public:
	ET_DECLARE_POINTER(MaterialInstance);
	using Collection = Vector<MaterialInstance::Pointer>;
	using Map = UnorderedMap<std::string, MaterialInstance::Pointer>;

public:
	MaterialInstance();
	MaterialInstance(Material::Pointer base);

	Material::Pointer base();

	void setTexture(MaterialTexture, Texture::Pointer);
	void setSampler(MaterialTexture, Sampler::Pointer);

	void setFloat(MaterialParameter, float);
	void setVector(MaterialParameter, const vec4&);

	void serialize(Dictionary, const std::string& baseFolder);

	uint64_t sortingKey() const;

private:
	std::array<Texture::Pointer, MaterialTexture_Max> _textures;
	std::array<vec4, MaterialParameter_Max> _parameters;
	Material::Pointer _base;
};

}
