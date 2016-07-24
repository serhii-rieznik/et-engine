/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/baseelement.h>

namespace et
{
	namespace s3d
	{
		class SkeletonElement : public BaseElement
		{
		public:
			ET_DECLARE_POINTER(SkeletonElement);
			
		public:
			SkeletonElement(const std::string& name, BaseElement* parent) :
				BaseElement(name, parent) { }

			ElementType type() const 
				{ return ElementType::Skeleton; }

			SkeletonElement* duplicate();
			
			mat4 calculateTransform();
		};
	}
}
