/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/camera/light.h>
#include <et/scene3d/baseelement.h>

namespace et
{
	namespace s3d
	{
		class LightElement : public Element, public Light
		{
		public:
			ET_DECLARE_POINTER(LightElement)

		public:
			LightElement(const std::string& name, Element* parent);

			ElementType type() const 
				{ return ElementType_Light; }

			LightElement* duplicate();

			Light& light()
				{ return *this; }

			const Light& light() const
				{ return *this; }

			void serialize(std::ostream& stream, SceneVersion version);
			void deserialize(std::istream& stream, ElementFactory* factory, SceneVersion version);
		};
	}
}
