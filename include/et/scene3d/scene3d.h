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
#include <et/scene3d/particlesystem.h>
#include <et/scene3d/lineelement.h>
#include <et/scene3d/light.h>
#include <et/scene3d/skeletonelement.h>
#include <et/scene3d/mesh.h>

namespace et
{
	namespace s3d
	{		
		class Scene : public ElementContainer, public SerializationHelper
		{
		public:
			ET_DECLARE_POINTER(Scene);
						
		public:
			Scene(const std::string& name = "scene");

			Storage& storage()
				{ return _storage; }

			const Storage& storage() const
				{ return _storage; }

		public:
			ET_DECLARE_EVENT1(deserializationFinished, bool)

		private:
			void cleanupGeometry();

			/*
			SceneMaterial::Pointer sceneMaterialWithName(const std::string&) override;
			Material::Pointer materialWithName(const std::string&) override;
			*/

			BaseElement::Pointer createElementOfType(ElementType, BaseElement*) override;

			const std::string& serializationBasePath() const override
				{ return _serializationBasePath; }

			IndexArray::Pointer indexArrayWithName(const std::string&) override;
			VertexStorage::Pointer vertexStorageWithName(const std::string&) override;
			
			VertexStream::Pointer vertexStreamWithStorageName(const std::string&) override;
						
		private:
			Storage _storage;
			// MaterialProvider* _currentMaterialProvider = nullptr;
			std::string _serializationBasePath;
			IndexBuffer::Pointer _mainIndexBuffer;
		};
	}
}
