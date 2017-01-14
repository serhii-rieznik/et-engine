/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/lightelement.h>

namespace et
{
namespace s3d
{

LightElement::LightElement(BaseElement* parent) 
	: BaseElement("light", parent)
	, _light(Light::Pointer::create(Light::Type::Directional))
{
}

LightElement::LightElement(const Light::Pointer& light, BaseElement* parent) 
	: BaseElement("light", parent)
	, _light(light)
{
}

BaseElement* LightElement::duplicate()
{
	ET_FAIL("TODO"); 
	return nullptr;
}

}
}
