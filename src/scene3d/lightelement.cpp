/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/lightelement.h>

using namespace et;
using namespace et::s3d;

LightElement::LightElement(const std::string& name, BaseElement* parent) :
	CameraElement(name, parent)
{
}

LightElement* LightElement::duplicate()
{
	LightElement* result = sharedObjectFactory().createObject<LightElement>(name(), parent());

	duplicateBasePropertiesToObject(result);
	duplicateChildrenToObject(result);

	result->setModelViewMatrix(modelViewMatrix());
	result->setProjectionMatrix(projectionMatrix());
	
	if (upVectorLocked())
		result->lockUpVector(lockedUpVector());

	return result;
}

void LightElement::serialize(Dictionary stream, const std::string& basePath)
{
	CameraElement::serialize(stream, basePath);
}

void LightElement::deserialize(Dictionary stream, SerializationHelper* helper)
{
	CameraElement::deserialize(stream, helper);
}

