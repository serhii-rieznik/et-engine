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
#include <et/rendering/base/vertexdeclaration.h>

namespace et
{

enum class MaterialTexture : uint32_t
{
	Albedo,
	Reflectance,
	Emissive,

	Roughness,
	Opacity,
	Normal,

	Count
};

enum class MaterialParameter : uint32_t
{
	AlbedoColor,
	ReflectanceColor,
	EmissiveColor,

	Roughness,
	Opacity,
	NormalScale,

	IndexOfRefraction,
	SpecularExponent,
	
	Count
};

const std::string& materialTextureToString(MaterialTexture);
const std::string& materialSamplerToString(MaterialTexture);

enum : uint32_t
{
	MaterialTexturesCount = static_cast<uint32_t>(MaterialTexture::Count),
	MaterialParametersCount = static_cast<uint32_t>(MaterialParameter::Count),
};

class RenderInterface;
class MaterialInstance;
using MaterialInstancePointer = IntrusivePtr<MaterialInstance>;

class Material : public Object
{
public:
	ET_DECLARE_POINTER(Material);

	template <class T>
	struct OptionalObject
	{
		T object;
		std::string name;
		uint32_t index = 0;
		bool assigned = false;

		void clear()
		{
			index = 0;
			object = nullptr;
			name.clear();
			assigned = false;
		}
	};

	struct OptionalValue
	{
		DataType storedType = DataType::max;
		char data[sizeof(vec4)] { };

		template <class T>
		const T& as() const
		{
			static_assert(sizeof(T) <= sizeof(vec4), "Requested type is too large");
			ET_ASSERT(is<T>());
			return *(reinterpret_cast<const T*>(data));
		};

		template <class T>
		bool is() const
		{
			static_assert(sizeof(T) <= sizeof(vec4), "Requested type is too large");
			return storedType == dataTypeFromClass<T>();
		}

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

	using Textures = std::array<OptionalObject<Texture::Pointer>, MaterialTexturesCount>;
	using Samplers = std::array<OptionalObject<Sampler::Pointer>, MaterialTexturesCount>;
	using Parameters = std::array<OptionalValue, MaterialParametersCount>;

public:
	Material(RenderInterface*);

	MaterialInstancePointer instance();

	void setTexture(MaterialTexture, Texture::Pointer);
	void setSampler(MaterialTexture, Sampler::Pointer);

	Texture::Pointer texture(MaterialTexture);
	Sampler::Pointer sampler(MaterialTexture);

	void setVector(MaterialParameter, const vec4&);
	vec4 getVector(MaterialParameter) const;

	void setFloat(MaterialParameter, float);
	float getFloat(MaterialParameter) const;

	const Textures& allTextures() const
		{ return _textures; }

	const Samplers& allSamplers() const
		{ return _samplers; }

	const Parameters& allParameters() const
		{ return _params; }

	Program::Pointer program();
	
	const DepthState& depthState() const
		{ return _depthState; }

	const BlendState& blendState() const
		{ return _blendState; };

	CullMode cullMode() const
		{ return _cullMode; }

	uint64_t sortingKey() const;

	void loadFromJson(const std::string& json, const std::string& baseFolder);
	
private:
	friend class MaterialInstance;

	template <class T>
	T getParameter(MaterialParameter) const;

	void loadInputLayout(Dictionary);
	void loadCode(const std::string&, const std::string& baseFolder, Dictionary defines);
	void generateInputLayout(std::string& code);

private: // overrided by instanaces
	Textures _textures;
	Samplers _samplers;
	Parameters _params;

private: // permanent private data
	RenderInterface* _renderer = nullptr;
	Program::Pointer _program;
	VertexDeclaration _inputLayout;
	DepthState _depthState;
	BlendState _blendState;
	CullMode _cullMode = CullMode::Disabled;
};

class MaterialInstance : public Material
{
public:
	ET_DECLARE_POINTER(MaterialInstance);
	using Collection = Vector<MaterialInstance::Pointer>;
	using Map = UnorderedMap<std::string, MaterialInstance::Pointer>;

public:
	Material::Pointer base();

private:
	friend class Material;
	friend class ObjectFactory;
	MaterialInstance(Material::Pointer base);

private:
	Material::Pointer _base;
};

std::string materialParameterToString(MaterialParameter);

template <class T>
inline T Material::getParameter(MaterialParameter p) const
{
	uint32_t pIndex = static_cast<uint32_t>(p);
	if (_params[pIndex].is<T>())
		return _params[pIndex].as<T>();
	
	log::warning("Material %s does not contain parameter %s of type %s", name().c_str(),
		materialParameterToString(p).c_str(), classToString<T>());
	return T();
}



}
