/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/scenematerial.h>
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

			SceneMaterial::Pointer& material()
				{ return _material; }

			const SceneMaterial::Pointer& material() const
				{ return _material; }

			void setMaterial(const SceneMaterial::Pointer& material)
				{ _material = material; }

			bool visible() const
				{ return _visible; }
			
			void setVisible(bool visible)
				{ _visible = visible; }

			void serialize(Dictionary, const std::string&);
			void deserialize(Dictionary, SerializationHelper*);
			
		private:
			SceneMaterial::Pointer _material;
			bool _visible = true;
		};
	}
}
