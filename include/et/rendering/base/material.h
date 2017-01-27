/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/rendering.h>
#include <et/rendering/base/materialhelpers.h>
#include <et/rendering/base/constantbuffer.h>

namespace et
{
class RenderInterface;
class MaterialInstance;
using MaterialInstancePointer = IntrusivePtr<MaterialInstance>;
using MaterialInstanceCollection = Vector<MaterialInstancePointer>;
class Material : public LoadableObject
{
public:
	ET_DECLARE_POINTER(Material);

	struct Configuration
	{
		Program::Pointer program;
		VertexDeclaration inputLayout;
		DepthState depthState;
		BlendState blendState;
		CullMode cullMode = CullMode::Disabled;
		StringList usedFiles;
	};
	using ConfigurationMap = Map<RenderPassClass, Configuration>;

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

	uint64_t sortingKey() const;

	virtual const Configuration& configuration(RenderPassClass) const;
	const ConfigurationMap& configurations() const { return _passes; }

	void loadFromJson(const std::string& json, const std::string& baseFolder);
	void setProgram(const Program::Pointer&, RenderPassClass);
	void setDepthState(const DepthState&, RenderPassClass);
	void setBlendState(const BlendState&, RenderPassClass);
	void setCullMode(CullMode, RenderPassClass);

private:
	friend class MaterialInstance;

	template <class T>
	T getParameter(MaterialParameter) const;

	VertexDeclaration loadInputLayout(Dictionary);
	Program::Pointer loadCode(const std::string&, const std::string& baseFolder, 
		Dictionary defines, const VertexDeclaration&, StringList& fileNames);
	std::string generateInputLayout(const VertexDeclaration& decl);

	void loadRenderPass(const std::string&, const Dictionary&, const std::string& baseFolder);
	void initDefaultHeader();

	virtual void invalidateTextureSet();
	virtual void invalidateConstantBuffer();

protected: // overrided / read by instanaces
	MaterialTextureSet textures;
	MaterialSamplerSet samplers;
	MaterialParameterSet properties;

private: // permanent private data
	static std::string _shaderDefaultHeader;
	RenderInterface* _renderer = nullptr;
	MaterialInstanceCollection _instances;
	ConfigurationMap _passes;
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
	const Configuration& configuration(RenderPassClass) const override;

	void invalidateTextureSet() override;
	void invalidateConstantBuffer() override;

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
	auto i = properties.find(pIndex);
	return ((i != properties.end()) && i->second.is<T>()) ? i->second.as<T>() : T();
}

}
