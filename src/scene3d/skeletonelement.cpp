/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/skeletonelement.h>

using namespace et;
using namespace et::s3d;

SkeletonElement* SkeletonElement::duplicate()
{
	SkeletonElement* result = sharedObjectFactory().createObject<SkeletonElement>(name(), parent());
	duplicateChildrenToObject(result);
	result->tag = tag;
	return result;
}

mat4 SkeletonElement::calculateTransform()
{
	if ((parent() == nullptr) || (parent()->type() != s3d::ElementType::Skeleton))
		return transform();
	
	return transform() * static_cast<SkeletonElement*>(parent())->calculateTransform();
}
