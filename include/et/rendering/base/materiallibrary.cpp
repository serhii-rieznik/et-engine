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

struct MaterialReloaded : public ObjectLoader
{
	ET_DECLARE_POINTER(MaterialReloaded);

	void reloadObject(LoadableObject::Pointer object, ObjectsCache&) override
	{
		Material::Pointer mtl = object;
		mtl->releaseInstances();
		mtl->loadFromJson(loadTextFile(mtl->origin()), getFilePath(mtl->origin()));
	}
};

const std::string defaultMaterials[DefaultMaterialCount] =
{
	std::string("engine_data/materials/textured2d.json"),
	std::string("engine_data/materials/textured.json"),
	std::string("engine_data/materials/microfacet.json"),
	std::string("engine_data/materials/environment.json"),
};

void MaterialLibrary::init(RenderInterface* ren)
{
	_renderer = ren;
	_cache.startMonitoring();
}

void MaterialLibrary::shutdown()
{
	for (Material::Pointer& mat : _defaultMaterials)
		mat.reset(nullptr);

	for (Material::Pointer& mtl : _loadedMaterials)
	{
		mtl->releaseInstances();
		_cache.discard(mtl);
	}
	
	_loadedMaterials.clear();
}

void MaterialLibrary::reloadMaterials()
{
	for (Material::Pointer mtl : _loadedMaterials)
	{
		mtl->releaseInstances();
		mtl->loadFromJson(loadTextFile(mtl->origin()), getFilePath(mtl->origin()));
	}
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
	Material::Pointer material = _cache.findAnyObject(fileName);
	
	if (material.invalid())
	{
		material = loadMaterialFromJson(loadTextFile(fileName), getFilePath(fileName));
		material->setOrigin(fileName);
		
		for (const auto& cfg : material->configurations())
		{
			for (const std::string& fn : cfg.second.usedFiles)
				material->addOrigin(fn);
		}

		_cache.manage(material, MaterialReloaded::Pointer::create());
	}
	
	return material;
}

Material::Pointer MaterialLibrary::loadMaterialFromJson(const std::string& json, const std::string& baseFolder)
{
	Material::Pointer result = Material::Pointer::create(_renderer);
	result->loadFromJson(json, baseFolder);
	_loadedMaterials.emplace_back(result);
	return result;
}

}
