/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/camera/camera.h>
#include <et/gui/guibase.h>

namespace et
{
	namespace gui
	{
		class Element3D : public Element
		{
		public:
			ET_DECLARE_POINTER(Element3D)

		public:
			Element3D(const Camera& camera, Element* parent, const std::string& name = std::string());

			virtual ElementRepresentation representation() const
				{ return ElementRepresentation_3d; };

			void setTransform(const mat4& transform);
			void applyTransform(const mat4& transform);

			const mat4& transform() const 
				{ return _transform; }

			const mat4& inverseFinalTransform();

			const vec2& position() const 
				{ return _null; }
			
			const vec2& pivotPoint() const
				{ return _null; }

			void setPosition(const vec2&, float)
				{ }

			void setSize(const vec2&, float)
				{ }

			void setFrame(const vec2&, const vec2&, float)
				{ }

			void setPivotPoint(const vec2&, bool)
				{ }

		protected:
			const mat4& finalTransform();
			void buildFinalTransform();

			const Camera& camera() const 
				{ return _camera; }

		private:
			const Camera& _camera;

			vec2 _null;
			
			mat4 _transform;
			mat4 _finalTransform;
			mat4 _inverseFinalTransform;

			bool _finalTransformValid;
			bool _inverseFinalTransformValid;
		};
	}
}
