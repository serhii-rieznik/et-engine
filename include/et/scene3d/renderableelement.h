/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/renderbatch.h>
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
			RenderableElement(const std::string& name, BaseElement* parent);
			RenderableElement(const std::string& name, const SceneMaterial::Pointer& mat, BaseElement* parent);
			
			SceneMaterial::Pointer& material()
				{ return _material; }

			const SceneMaterial::Pointer& material() const
				{ return _material; }
			void setMaterial(const SceneMaterial::Pointer& material)
				{ _material = material; }
			
			void addRenderBatch(RenderBatch::Pointer);
			void prepareRenderBatches();
			
			std::vector<RenderBatch::Pointer>& renderBatches();
			const std::vector<RenderBatch::Pointer>& renderBatches() const;

			void serialize(Dictionary, const std::string&);
			void deserialize(Dictionary, SerializationHelper*);
			
		protected:
			// should use RenderBatches' transformation
			const mat4& finalInverseTransform();
			const mat4& finalTransform();
			
		private:
			SceneMaterial::Pointer _material;
			std::vector<RenderBatch::Pointer> _renderBatches;
		};
	}
}
