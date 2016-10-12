/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/datastorage.h>
#include <et/rendering/interface/program.h>
#include <et/rendering/interface/texture.h>

namespace et
{
class RenderInterface;

class Material : public LoadableObject
{
public:
	ET_DECLARE_POINTER(Material);
	
public:
	Material(RenderInterface*);
	
	void loadFromJson(const std::string&, const std::string& baseFolder);

	void setBlendState(const BlendState&);
	void setDepthState(const DepthState&);
	void setCullMode(CullMode);
	void setProgram(Program::Pointer);
	
	et::Program::Pointer& program()
		{ return _program; }
	
	const et::Program::Pointer& program() const
		{ return _program; }
	
	const DepthState& depthState() const
		{ return _depth; }
	
	const BlendState& blendState() const
		{ return _blend; }
	
	CullMode cullMode() const
		{ return _cull; }

	uint32_t sortingKey() const;

public:
	struct Property
	{
		uint32_t length = 0;
		char data[sizeof(mat4)] { };
		Property(uint32_t sz)
			: length(sz) { }
	};
	using PropertyMap = UnorderedMap<String, Property>;

	template <class T>
	void setProperty(const String& name, const T& value);
	const PropertyMap& properties() const
		{ return _properties; }

	static const String kAlbedo;
	using TextureMap = UnorderedMap<String, Texture::Pointer>;

	void setTexutre(const String& name, const Texture::Pointer&);
	Texture::Pointer texture(const String& name) const;
	
	const TextureMap& textures() const
		{ return _textures; }

private:
	void uploadPropertyData(Property& prop, const void* src, uint32_t sz);

public:
	RenderInterface* _renderer = nullptr;
	PropertyMap _properties;
	TextureMap _textures;
	Program::Pointer _program;
	DepthState _depth;
	BlendState _blend;
	CullMode _cull = CullMode::Disabled;
	uint32_t _additionalPriority = 0;
};

template <class T>
inline void Material::setProperty(const String& aName, const T& value)
{
	auto i = _properties.find(aName);
	if (i == _properties.end())
	{
		i = _properties.emplace(aName, sizeof(value)).first;
	}
	uploadPropertyData(i->second, &value, sizeof(value));
}

}
