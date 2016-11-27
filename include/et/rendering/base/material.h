/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/rendering.h>
#include <et/rendering/base/materialhelpers.h>
#include <et/rendering/constantbuffer.h>

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
	void flushInstances();
	void releaseInstances();

	void setTexture(MaterialTexture, Texture::Pointer);
	void setSampler(MaterialTexture, Sampler::Pointer);

	Texture::Pointer texture(MaterialTexture);
	Sampler::Pointer sampler(MaterialTexture);

	void setVector(MaterialParameter, const vec4&);
	vec4 getVector(MaterialParameter) const;

	void setFloat(MaterialParameter, float);
	float getFloat(MaterialParameter) const;

	Program::Pointer program(RenderPassClass) const;

	const DepthState& depthState() const
		{ return _depthState; }

	const BlendState& blendState() const
		{ return _blendState; };

	CullMode cullMode() const
		{ return _cullMode; }

	void setProgram(const Program::Pointer&, RenderPassClass);
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
	void loadCode(const Dictionary & codes, const std::string & baseFolder, Dictionary defines);
	void loadCode(const std::string&, RenderPassClass passCls, const std::string& baseFolder, Dictionary defines);
	std::string generateInputLayout();

protected: // overrided / read by instanaces
	mtl::Textures textures;
	mtl::Samplers samplers;
	mtl::Parameters properties;

private: // permanent private data
	RenderInterface* _renderer = nullptr;
	std::map<RenderPassClass, Program::Pointer> _programs;
	MaterialInstanceCollection _instances;
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

	TextureSet::Pointer textureSet(RenderPassClass);
	ConstantBufferEntry constantBufferData(RenderPassClass);

	void invalidateTextureSet();
	void invalidateConstantBuffer();

private:
	friend class Material;
	friend class ObjectFactory;
	MaterialInstance(Material::Pointer base);

	void buildTextureSet(RenderPassClass);
	void buildConstantBuffer(RenderPassClass);

private:
	template <class T>
	struct Holder
	{
		T obj;
		bool valid = false;
	};

private:
	Material::Pointer _base;
	std::map<RenderPassClass, Holder<TextureSet::Pointer>> _textureSets;
	std::map<RenderPassClass, Holder<ConstantBufferEntry>> _constBuffers;
};

template <class T>
inline T Material::getParameter(MaterialParameter p) const
{
	uint32_t pIndex = static_cast<uint32_t>(p);
	return properties[pIndex].is<T>() ? properties[pIndex].as<T>() : T();
}

}
