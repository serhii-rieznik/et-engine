/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/materialhelpers.h>

namespace et
{

class RenderInterface;
class MaterialInstance;
using MaterialInstancePointer = IntrusivePtr<MaterialInstance>;
using MaterialInstanceCollection = Vector<MaterialInstancePointer>;

class Material : public Object
{
public:
	ET_DECLARE_POINTER(Material);

public:
	Material(RenderInterface*);

	MaterialInstancePointer instance();
	const MaterialInstanceCollection& instances() const;
	void releaseInstances();

	void setTexture(MaterialTexture, Texture::Pointer);
	void setSampler(MaterialTexture, Sampler::Pointer);

	Texture::Pointer texture(MaterialTexture);
	Sampler::Pointer sampler(MaterialTexture);

	void setVector(MaterialParameter, const vec4&);
	vec4 getVector(MaterialParameter) const;

	void setFloat(MaterialParameter, float);
	float getFloat(MaterialParameter) const;

	Program::Pointer program();
	
	const DepthState& depthState() const
		{ return _depthState; }

	const BlendState& blendState() const
		{ return _blendState; };

	CullMode cullMode() const
		{ return _cullMode; }

	void setProgram(Program::Pointer);
	void setDepthState(const DepthState&);
	void setBlendState(const BlendState&);
	void setCullMode(CullMode);

	uint64_t sortingKey() const;

	void loadFromJson(const std::string& json, const std::string& baseFolder);

private:
	friend class MaterialInstance;

	template <class T>
	T getParameter(MaterialParameter) const;

	void loadInputLayout(Dictionary);
	void loadCode(const std::string&, const std::string& baseFolder, Dictionary defines);
	void generateInputLayout(std::string& code);

protected: // overrided / read by instanaces
	mtl::Textures textures;
	mtl::Samplers samplers;
	mtl::Parameters properties;

private: // permanent private data
	RenderInterface* _renderer = nullptr;
	MaterialInstanceCollection _instances;
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

	const MaterialTexturesCollection& usedTextures();
	const MaterialSamplersCollection& usedSamplers();
	const MaterialPropertiesCollection& usedProperties();

	void invalidateUsedTextures();
	void invalidateUsedSamplers();
	void invalidateUsedProperties();

private:
	friend class Material;
	friend class ObjectFactory;
	MaterialInstance(Material::Pointer base);

	void buildUsedTextures();
	void buildUsedSamplers();
	void buildUsedProperties();

private:
	Material::Pointer _base;
	MaterialTexturesCollection _usedTextures;
	MaterialSamplersCollection _usedSamplers;
	MaterialPropertiesCollection _usedProperties;
	bool _texturesValid = false;
	bool _samplersValid = false;
	bool _propertiesValid = false;
};

template <class T>
inline T Material::getParameter(MaterialParameter p) const
{
	uint32_t pIndex = static_cast<uint32_t>(p);
	if (properties[pIndex].is<T>())
		return properties[pIndex].as<T>();
	
	log::warning("Material %s does not contain parameter %s of type %s", name().c_str(),
		mtl::materialParameterToString(p).c_str(), classToString<T>());
	return T();
}



}
