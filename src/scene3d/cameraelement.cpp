/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/conversion.h>
#include <et/scene3d/cameraelement.h>

using namespace et;
using namespace et::s3d;

CameraElement::CameraElement(const std::string& name, BaseElement* parent) : 
	BaseElement(name, parent)
{

}

CameraElement* CameraElement::duplicate()
{
	CameraElement* result = etCreateObject<CameraElement>(name(), parent());

	duplicateBasePropertiesToObject(result);
	duplicateChildrenToObject(result);

	result->setViewMatrix(viewMatrix());
	result->setProjectionMatrix(projectionMatrix());
	
	if (upVectorLocked())
		result->lockUpVector(lockedUpVector());

	return result;
}

void CameraElement::serialize(Dictionary stream, const std::string& basePath)
{
	const auto& mv = viewMatrix();
	const auto& pr = projectionMatrix();
	
	ArrayValue view;
	view->content.push_back(vec4ToArray(mv[0]));
	view->content.push_back(vec4ToArray(mv[1]));
	view->content.push_back(vec4ToArray(mv[2]));
	view->content.push_back(vec4ToArray(mv[3]));
	stream.setArrayForKey(kView, view);

	ArrayValue projection;
	projection->content.push_back(vec4ToArray(pr[0]));
	projection->content.push_back(vec4ToArray(pr[1]));
	projection->content.push_back(vec4ToArray(pr[2]));
	projection->content.push_back(vec4ToArray(pr[3]));
	stream.setArrayForKey(kProjection, projection);

	stream.setArrayForKey(kUpVector, vec3ToArray(lockedUpVector()));
	stream.setIntegerForKey(kUpVectorLocked, upVectorLocked());

	BaseElement::serialize(stream, basePath);
}

void CameraElement::deserialize(Dictionary stream, SerializationHelper* helper)
{
	BaseElement::deserialize(stream, helper);
	ET_FAIL("Not implemented");
}
