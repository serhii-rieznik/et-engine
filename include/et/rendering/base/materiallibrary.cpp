/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/base/materiallibrary.h>

namespace et
{

const std::string defaultMaterials[static_cast<uint32_t>(DefaultMaterial::max)] =
{
	std::string("engine_data/materials/textured.json"),
	std::string("engine_data/materials/phong.json"),
};

void MaterialLibrary::init(RenderInterface* ren)
{
	_renderer = ren;
}

void MaterialLibrary::shutdown()
{

}

Material::Pointer MaterialLibrary::loadDefaultMaterial(DefaultMaterial mtl)
{
	ET_ASSERT(mtl < DefaultMaterial::max);
	std::string fileName = application().resolveFileName(defaultMaterials[static_cast<uint32_t>(mtl)]);
	return loadMaterial(fileName);
}

Material::Pointer MaterialLibrary::loadMaterial(const std::string& fileName)
{
	return loadMaterialFromJson(loadTextFile(fileName), getFilePath(fileName));
}

Material::Pointer MaterialLibrary::loadMaterialFromJson(const std::string& json, const std::string& baseFolder)
{
	Material::Pointer result = Material::Pointer::create(_renderer);
	result->loadFromJson(json, baseFolder);
	return result;
}

}
