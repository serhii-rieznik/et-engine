/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/rendering.h>
#include <et/rendering/base/variableset.h>
#include <et/rendering/base/constantbuffer.h>
#include <et/rendering/interface/program.h>

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
	using ConfigurationMap = UnorderedMap<std::string, Configuration>;

public:
	Material(RenderInterface*);

	MaterialInstancePointer instance();
	const MaterialInstanceCollection& instances() const;
	void flushInstances();
	void releaseInstances();
	void invalidateInstances();

	void setTexture(const std::string&, const Texture::Pointer&, const ResourceRange& = ResourceRange::whole);
	void setSampler(const std::string&, const Sampler::Pointer&);
	void setImage(const std::string&, const Texture::Pointer&);

	const Texture::Pointer& texture(const std::string&);
	const Sampler::Pointer& sampler(const std::string&);
	const Texture::Pointer& image(const std::string&);

	void setVector(MaterialVariable, const vec4&);
	vec4 getVector(MaterialVariable) const;

	void setFloat(MaterialVariable, float);
	float getFloat(MaterialVariable) const;

	uint64_t sortingKey() const;

	const Configuration& configuration(const std::string&) const;
	const ConfigurationMap& configurations() const { return _configurations; }

	void loadFromJson(const std::string& json, const std::string& baseFolder);

	PipelineClass pipelineClass() const { return _pipelineClass; }

	virtual bool isInstance() const { return false; }

private:
	friend class MaterialInstance;

	template <class T>
	T getParameter(MaterialVariable) const;

	VertexDeclaration loadInputLayout(Dictionary);
	Program::Pointer loadCode(const std::string&, const std::string& baseFolder, 
		const Dictionary& defines, const VertexDeclaration&, StringList& fileNames);
	std::string generateInputLayout(const VertexDeclaration& decl);

	void setProgram(const Program::Pointer&, const std::string&);
	void setDepthState(const DepthState&, const std::string&);
	void setBlendState(const BlendState&, const std::string&);
	void setCullMode(CullMode, const std::string&);

	void loadRenderPass(const std::string&, const Dictionary&, const std::string& baseFolder);
	void initDefaultHeader();

	virtual void invalidateTextureBindingsSet();
	virtual void invalidateConstantBuffer();

protected: // overrided / read by instanaces
	TexturesHolder textures;
	SamplersHolder samplers;
	ImagesHolder images;
	VariablesHolder properties;

	virtual bool validateImageName(const std::string&);
	virtual bool validateTextureName(const std::string&);
	virtual bool validateSamplerName(const std::string&);

private: // permanent private data
	static std::string _shaderDefaultHeader;
	Set<std::string> _usedTextures;
	Set<std::string> _usedSamplers;
	Set<std::string> _usedImages;
	RenderInterface* _renderer = nullptr;
	MaterialInstanceCollection _activeInstances;
	MaterialInstanceCollection _instancesPool;
	ConfigurationMap _configurations;
	PipelineClass _pipelineClass = PipelineClass::Graphics;
	uint32_t _instancesCounter = 0;
};

class MaterialInstance : public Material
{
public:
	ET_DECLARE_POINTER(MaterialInstance);
	using Collection = Vector<MaterialInstance::Pointer>;
	using Map = UnorderedMap<std::string, MaterialInstance::Pointer>;

public:
	Material::Pointer& base();
	const Material::Pointer& base() const;

	const TextureSet::Pointer& textureBindingsSet(const std::string&);
	const ConstantBufferEntry::Pointer& constantBufferData(const std::string&);

	void invalidateTextureBindingsSet() override;
	void invalidateConstantBuffer() override;
	
	bool isInstance() const override { return true; }

	void serialize(std::ostream&) const;
	void deserialize(std::istream&);

	bool validateImageName(const std::string&) override;
	bool validateTextureName(const std::string&) override;
	bool validateSamplerName(const std::string&) override;

private:
	friend class Material;
	friend class ObjectFactory;

	template <class T> 
	struct Holder
	{
		T obj;
		bool valid = false;
	};

	MaterialInstance(Material::Pointer base);

	void buildTextureBindingsSet(const std::string&, Holder<TextureSet::Pointer>& holder);
	void buildConstantBuffer(const std::string&, Holder<ConstantBufferEntry::Pointer>& holder);

private:
	Material::Pointer _base;
	UnorderedMap<std::string, Holder<TextureSet::Pointer>> _textureBindingsSets;
	UnorderedMap<std::string, Holder<ConstantBufferEntry::Pointer>> _constBuffers;
};

template <class T>
inline T Material::getParameter(MaterialVariable p) const
{
	uint32_t pIndex = static_cast<uint32_t>(p);
	auto i = std::find_if(properties.begin(), properties.end(), [pIndex](const VariablesHolder::value_type& t) {
		return t.first == pIndex;
	});
	return ((i != properties.end()) && i->second.is<T>()) ? i->second.as<T>() : T();
}

}
