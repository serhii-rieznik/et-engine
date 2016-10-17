/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/base/material.h>

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

void Material::setSampler(MaterialTexture t, Sampler::Pointer s)
{
	_samplers[static_cast<size_t>(t)] = s;
}

void Material::setVector(MaterialParameter p, const vec4& v)
{
	_params[static_cast<size_t>(p)] = v;
}

void Material::setFloat(MaterialParameter p, float f)
{
	_params[static_cast<size_t>(p)] = f;
}

void Material::loadFromJson(const std::string& json, const std::string& baseFolder)
{
	
}

MaterialInstancePointer Material::instance()
{
	MaterialInstance::Pointer result = MaterialInstance::Pointer::create(Material::Pointer(this));
	result->_textures = _textures;
	result->_samplers = _samplers;
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

void MaterialInstance::serialize(Dictionary, const std::string& baseFolder)
{
}

}
