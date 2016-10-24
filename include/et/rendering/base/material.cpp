/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/base/material.h>
#include <et/core/json.h>

namespace et
{

/*
 * Material
 */
Material::Material(RenderInterface* ren)
	: _renderer(ren)
{
}

uint64_t Material::sortingKey() const
{
	return 0;
}

void Material::setTexture(MaterialTexture t, Texture::Pointer tex)
{
	_textures[static_cast<size_t>(t)] = tex;
}

void Material::setVector(MaterialParameter p, const vec4& v)
{
	_params[static_cast<size_t>(p)] = v;
}

vec4 Material::getVector(MaterialParameter p) const
{
	return getParameter<vec4>(p);
}

void Material::setFloat(MaterialParameter p, float f)
{
	_params[static_cast<size_t>(p)] = f;
}

float Material::getFloat(MaterialParameter p) const
{
	return getParameter<float>(p);
}

void Material::loadFromJson(const std::string& source, const std::string& baseFolder)
{
	VariantClass cls = VariantClass::Invalid;
	Dictionary obj = json::deserialize(source, cls);
	if (cls != VariantClass::Dictionary)
	{
		log::error("Unable to load material from JSON");
		return;
	}
}

MaterialInstancePointer Material::instance()
{
	MaterialInstance::Pointer result = MaterialInstance::Pointer::create(Material::Pointer(this));
	result->_textures = _textures;
	result->_params = _params;
	return result;
}

Program::Pointer Material::program()
{
	return _program;
}

/*
 * Material Instance
 */
MaterialInstance::MaterialInstance(Material::Pointer bs)
	: Material(bs->_renderer), _base(bs)
{
}

Material::Pointer MaterialInstance::base()
{
	return _base;
}

/*
 * Service
 */
#define MTL_CASE(X) case MaterialParameter::X: return #X;
std::string materialParameterToString(MaterialParameter p)
{
	switch (p)
	{
		MTL_CASE(AmbientColor)
		MTL_CASE(DiffuseColor)
		MTL_CASE(SpecularColor)
		MTL_CASE(EmissiveColor)
		MTL_CASE(Roughness)
		MTL_CASE(Opacity)
		MTL_CASE(NormalTextureScale)
		default:
			ET_FAIL_FMT("Invalid or unknown material parameter provided: %u", static_cast<uint32_t>(p));
	}
}
#undef MTL_CASE

}
