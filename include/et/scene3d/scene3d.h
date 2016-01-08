/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/core/objectscache.h>
#include <et/scene3d/elementcontainer.h>
#include <et/scene3d/storage.h>
#include <et/scene3d/cameraelement.h>
#include <et/scene3d/lightelement.h>
#include <et/scene3d/particlesystem.h>
#include <et/scene3d/lineelement.h>
#include <et/scene3d/skeletonelement.h>
#include <et/scene3d/mesh.h>

namespace et
{
	namespace s3d
	{		
		class Scene : public ElementContainer, public SerializationHelper
		{
		public:
			ET_DECLARE_POINTER(Scene)
						
		public:
			Scene(const std::string& name = "scene");

			Dictionary serialize(const std::string& basePath);
			
			void deserializeWithOptions(et::RenderContext*, MaterialProvider*, Dictionary,
				const std::string& basePath, ObjectsCache&, uint32_t options);

			Storage& storage()
				{ return _storage; }

			const Storage& storage() const
				{ return _storage; }

		public:
			ET_DECLARE_EVENT1(deserializationFinished, bool)

		private:
			void buildVertexBuffers(et::RenderContext*);
			void cleanupGeometry();
			void cleanUpSupportMehses();
			
			SceneMaterial::Pointer sceneMaterialWithName(const std::string&) override;
			Material::Pointer materialWithName(const std::string&) override;

			BaseElement::Pointer createElementOfType(ElementType, BaseElement*) override;

			const std::string& serializationBasePath() const override
				{ return _serializationBasePath; }

			IndexArray::Pointer indexArrayWithName(const std::string&) override;
			VertexStorage::Pointer vertexStorageWithName(const std::string&) override;
			
			VertexArrayObject::Pointer vertexArrayWithStorageName(const std::string&) override;
						
		private:
			Storage _storage;
			MaterialProvider* _currentMaterialProvider = nullptr;
			std::string _serializationBasePath;
			std::vector<VertexArrayObject::Pointer> _vertexArrayObjects;
			IndexBuffer::Pointer _mainIndexBuffer;
		};
	}
}
