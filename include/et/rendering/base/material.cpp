/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/base/material.h>

namespace et
{

MaterialInstance::MaterialInstance()
{
}

MaterialInstance::MaterialInstance(Material::Pointer bs) : _base(bs)
{
}

Material::Pointer MaterialInstance::base()
{
	return _base;
}

void MaterialInstance::setTexture(MaterialTexture, Texture::Pointer)
{
}

void MaterialInstance::setSampler(MaterialTexture, Sampler::Pointer)
{
}

void MaterialInstance::setFloat(MaterialParameter, float)
{
}

void MaterialInstance::setVector(MaterialParameter, const vec4&)
{
}

void MaterialInstance::serialize(Dictionary, const std::string& baseFolder)
{
}

uint64_t MaterialInstance::sortingKey() const
{
	return 0;
}

}
