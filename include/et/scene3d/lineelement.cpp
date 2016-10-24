/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/lineelement.h>

using namespace et;
using namespace et::s3d;

LineElement::LineElement(const std::string& name, BaseElement* parent) : 
	ElementContainer(name, parent) 
{
	_points.reserve(16);
}

void LineElement::addPoint(const vec3& p)
{
	_points.push_back(p);
}

LineElement* LineElement::duplicate()
{
	ET_FAIL("Not implemented")
}
