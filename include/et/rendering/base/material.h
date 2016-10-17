/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/texture.h>
#include <et/rendering/interface/sampler.h>
#include <et/rendering/interface/program.h>

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

class RenderInterface;
class MaterialInstance;
using MaterialInstancePointer = IntrusivePtr<MaterialInstance>;

class Material : public Object
{
public:
	ET_DECLARE_POINTER(Material);

	template <class T>
	struct MaterialOptionalObject
	{
		T value = T(0);
		bool assigned = false;

		void operator = (const T& v)
		{
			value = v;
			assigned = true;
		}

		void clear()
		{
			value = T(0);
			assigned = false;
		}
	};

	struct MaterialOptionalValue
	{
		DataType storedType = DataType::max;
		char data[sizeof(vec4)] { };

		template <class T>
		const T& as()
		{
			static_assert(sizeof(T) <= sizeof(vec4), "Requested type is too large");
			ET_ASSERT(storedType == dataTypeFromClass<T>());
			return *(reinterpret_cast<T*>(data));
		};

		template <class T>
		void operator = (const T& value)
		{
			static_assert(sizeof(T) <= sizeof(vec4), "Requested type is too large");
			*(reinterpret_cast<T*>(data)) = value;
			storedType = dataTypeFromClass<T>();
		}

		void clear()
		{
			memset(data, 0, sizeof(data));
			storedType = DataType::max;
		}
	};

	using Textures = std::array<MaterialOptionalObject<Texture::Pointer>, MaterialTexture_Max>;
	using Samplers = std::array<MaterialOptionalObject<Sampler::Pointer>, MaterialTexture_Max>;
	using Parameters = std::array<MaterialOptionalValue, MaterialParameter_Max>;

public:
	Material(RenderInterface*);

	MaterialInstancePointer instance();

	void setTexture(MaterialTexture, Texture::Pointer);
	void setSampler(MaterialTexture, Sampler::Pointer);
	void setVector(MaterialParameter, const vec4&);
	void setFloat(MaterialParameter, float);

	Program::Pointer program();

	uint64_t sortingKey() const;

	void loadFromJson(const std::string& json, const std::string& baseFolder);
	
private:
	friend class MaterialInstance;

private:
	RenderInterface* _renderer = nullptr;
	Textures _textures;
	Samplers _samplers;
	Parameters _params;
	Program::Pointer _program;
};

class MaterialInstance : public Material
{
public:
	ET_DECLARE_POINTER(MaterialInstance);
	using Collection = Vector<MaterialInstance::Pointer>;
	using Map = UnorderedMap<std::string, MaterialInstance::Pointer>;

public:
	Material::Pointer base();

	void serialize(Dictionary, const std::string& baseFolder);

private:
	friend class Material;
	friend class ObjectFactory;
	MaterialInstance(Material::Pointer base);

private:
	Material::Pointer _base;
};

}
