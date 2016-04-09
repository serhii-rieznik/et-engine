/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/geometry.h>
#include <et/scene3d/elementcontainer.h>

namespace et
{
	namespace s3d
	{
		class LineElement : public ElementContainer
		{
		public:
			ET_DECLARE_POINTER(LineElement)
			
		public:
			LineElement(const std::string& name, BaseElement* parent);

			ElementType type() const 
				{ return ElementType::Line; }

			void serialize(Dictionary, const std::string&);
			void deserialize(Dictionary, SerializationHelper*);
			LineElement* duplicate();

			const std::vector<vec3>& points() const 
				{ return _points; }

			void addPoint(const vec3&);

		private:
			std::vector<vec3> _points;
		};
	}
}
