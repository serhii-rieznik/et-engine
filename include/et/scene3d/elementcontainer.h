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
		class ElementContainer : public BaseElement
		{
		public:
			ET_DECLARE_POINTER(ElementContainer)
			
		public:
			ElementContainer(const std::string& name, BaseElement* parent) :
				BaseElement(name, parent) { }

			ElementType type() const 
				{ return ElementType::Container; }

			ElementContainer* duplicate()
			{
				ElementContainer* result = etCreateObject<ElementContainer>(name(), parent());
				duplicateChildrenToObject(result);
				return result; 
			}
		};
	}
}
