/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/base/materiallibrary.h>

namespace et
{

void MaterialLibrary::init(RenderInterface* ren)
{
	_renderer = ren;
}

void MaterialLibrary::shutdown()
{

}

Material::Pointer MaterialLibrary::loadMaterial(const std::string& fileName)
{
	return loadMaterialFromJson(loadTextFile(fileName));
}

Material::Pointer MaterialLibrary::loadMaterialFromJson(const std::string& json)
{
	Material::Pointer result = Material::Pointer::create(_renderer);

	return result;
}

}
