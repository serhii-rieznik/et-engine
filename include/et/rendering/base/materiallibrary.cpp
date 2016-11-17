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

const std::string defaultMaterials[DefaultMaterialCount] =
{
	std::string("engine_data/materials/textured2d.json"),
	std::string("engine_data/materials/textured.json"),
	std::string("engine_data/materials/microfacet.json"),
};

void MaterialLibrary::init(RenderInterface* ren)
{
	_renderer = ren;
}

void MaterialLibrary::shutdown()
{
	for (Material::Pointer& mat : _defaultMaterials)
	{
		if (mat.valid())
		{
			mat->releaseInstances();
			mat.reset(nullptr);
		}
	}

	for (auto& kv : _cache)
		kv.second->releaseInstances();
	_cache.clear();
}

Material::Pointer MaterialLibrary::loadDefaultMaterial(DefaultMaterial mtl)
{
	ET_ASSERT(mtl < DefaultMaterial::Count);

	uint32_t mtlIndex = static_cast<uint32_t>(mtl);
	if (_defaultMaterials[mtlIndex].invalid())
	{
		std::string fileName = application().resolveFileName(defaultMaterials[mtlIndex]);
		_defaultMaterials[mtlIndex] = loadMaterial(fileName);
	}
	return _defaultMaterials[mtlIndex];
}

Material::Pointer MaterialLibrary::loadMaterial(const std::string& fileName)
{
	auto i = _cache.find(fileName);
	if (i != _cache.end())
		return i->second;

	Material::Pointer mtl = loadMaterialFromJson(loadTextFile(fileName), getFilePath(fileName));
	return _cache.emplace(fileName, mtl).first->second;
}

Material::Pointer MaterialLibrary::loadMaterialFromJson(const std::string& json, const std::string& baseFolder)
{
	Material::Pointer result = Material::Pointer::create(_renderer);
	result->loadFromJson(json, baseFolder);
	return result;
}

}
