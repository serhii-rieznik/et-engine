/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/material.h>

namespace et
{

enum class DefaultMaterial : uint32_t
{
	Textured2D,
	Textured,
	Microfacet,
	
	Count
};

enum : uint32_t
{
	DefaultMaterialCount = static_cast<uint32_t>(DefaultMaterial::Count)
};

class RenderInterface;
class MaterialLibrary
{
public:
	void init(RenderInterface*);
	void shutdown();

	Material::Pointer loadDefaultMaterial(DefaultMaterial mtl);
	Material::Pointer loadMaterial(const std::string& fileName);
	Material::Pointer loadMaterialFromJson(const std::string& json, const std::string& baseFolder);

private:
	RenderInterface* _renderer = nullptr;
	Map<std::string, Material::Pointer> _cache;
	std::array<Material::Pointer, DefaultMaterialCount> _defaultMaterials;
};

}
