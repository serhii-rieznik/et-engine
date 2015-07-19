/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/material.h>
#include <et/scene3d/elementcontainer.h>

namespace et
{
	namespace s3d
	{
		class RenderableElement : public ElementContainer
		{
		public:
			ET_DECLARE_POINTER(RenderableElement)
			
		public:
			RenderableElement(const std::string& name, BaseElement* parent) :
				ElementContainer(name, parent) {
				setFlag(Flag_Renderable);
			}

			Material::Pointer& material()
				{ return _material; }

			const Material::Pointer& material() const
				{ return _material; }

			void setMaterial(const Material::Pointer& material)
				{ _material = material; }

			bool visible() const
				{ return _visible; }
			
			void setVisible(bool visible)
				{ _visible = visible; }

			void serialize(Dictionary, const std::string&);
			void deserialize(Dictionary, SerializationHelper*);
			
		private:
			Material::Pointer _material;
			bool _visible = true;
		};
	}
}
