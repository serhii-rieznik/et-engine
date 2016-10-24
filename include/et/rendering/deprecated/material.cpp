/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/shadersource.h>
#include <et/rendering/interface/renderer.h>

using namespace et;

namespace
{
const std::string kProgramSource = "program-source";
const std::string kDefines = "defines";
const std::string kRenderPriority = "render-priority";
}

const String Material::kAlbedo = "albedoTexture";

Material::Material(RenderInterface* renderer) :
	_renderer(renderer)
{
}

void Material::loadFromJson(const std::string& jsonString, const std::string& baseFolder)
{
	Dictionary obj;
	if (obj.loadFromJson(jsonString) == false)
	{
		log::error("Unable to load material from json.");
		return;
	}

	
	auto name = obj.stringForKey(kName)->content;
	auto cullMode = obj.stringForKey(kCullMode)->content;
	auto definesArray = obj.arrayForKey(kDefines);
	
	setName(name);
	setOrigin(baseFolder);

	_additionalPriority = static_cast<uint32_t>(obj.integerForKey(kRenderPriority, 0ll)->content);

	setBlendState(deserializeBlendState(obj.dictionaryForKey(kBlendState)));
	setDepthState(deserializeDepthState(obj.dictionaryForKey(kDepthState)));

	CullMode cm = CullMode::Disabled;
	if (stringToCullMode(cullMode, cm) == false)
	{
		log::warning("Invalid or unsupported cull mode in material: %s", cullMode.c_str());
	}
	setCullMode(cm);


}

void Material::setBlendState(const BlendState& state)
{
	_blend = state;
}

void Material::setDepthState(const DepthState& state)
{
	_depth = state;
}

void Material::setCullMode(CullMode cm)
{
	_cull = cm;
}

void Material::setProgram(Program::Pointer program)
{
	_program = program;
}

void Material::uploadPropertyData(Property& prop, const void* src, uint32_t sz)
{
	ET_ASSERT(sz == prop.length);
	memcpy(prop.data, src, sz);
}

void Material::setTexutre(const String& name, const Texture::Pointer& tex)
{
	_textures[name] = tex;
}

Texture::Pointer Material::texture(const String& name) const
{
	auto i = _textures.find(name);
	return (i == _textures.end()) ? Texture::Pointer() : i->second;
}

uint32_t Material::sortingKey() const
{
	return _depth.sortingKey() | _blend.sortingKey() << 8 | _additionalPriority << 16;
}
