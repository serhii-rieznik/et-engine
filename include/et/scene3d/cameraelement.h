/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/camera/camera.h>
#include <et/scene3d/baseelement.h>

namespace et
{
	namespace s3d
	{
		class CameraElement : public Element, public Camera
		{
		public:
			ET_DECLARE_POINTER(CameraElement)
			
		public:
			CameraElement(const std::string& name, Element* parent);

			ElementType type() const 
				{ return ElementType_Camera; }

			CameraElement* duplicate();

			Camera& camera()
				{ return *this; }

			const Camera& camera() const
				{ return *this; }

			void serialize(std::ostream& stream, SceneVersion version);
			void deserialize(std::istream& stream, ElementFactory* factory, SceneVersion version);
		};
	}
}