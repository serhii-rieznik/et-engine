/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/renderableelement.h>

using namespace et;
using namespace et::s3d;

void RenderableElement::serialize(Dictionary stream, const std::string& basePath)
{
	stream.setStringForKey(kMaterialName, material()->name());
	ElementContainer::serialize(stream, basePath);
}

void RenderableElement::deserialize(Dictionary stream, SerializationHelper* helper)
{
	auto materialName = stream.stringForKey(kMaterialName)->content;

	SceneMaterial::Pointer material(helper->materialWithName(materialName));
	setMaterial(material);

	ElementContainer::deserialize(stream, helper);
}
