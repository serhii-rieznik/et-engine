/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/cameraelement.h>

namespace et
{
	namespace s3d
	{
		class LightElement : public CameraElement
		{
		public:
			ET_DECLARE_POINTER(LightElement)

		public:
			LightElement(const std::string& name, BaseElement* parent);

			ElementType type() const 
				{ return ElementType_Light; }

			LightElement* duplicate();

			void serialize(Dictionary, const std::string&);
			void deserialize(Dictionary, ElementFactory* factory);
		};
	}
}
