/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/core/objectscache.h>
#include <et/scene3d/baseelement.h>
#include <et/scene3d/storage.h>
#include <et/scene3d/mesh.h>
#include <et/scene3d/supportmesh.h>
#include <et/scene3d/cameraelement.h>
#include <et/scene3d/lightelement.h>
#include <et/scene3d/particlesystem.h>

namespace et
{
	namespace s3d
	{
		class Scene : public EventReceiver, public ElementContainer, public ElementFactory
		{
		public:
			ET_DECLARE_POINTER(Scene)
			
		public:
			Scene(const std::string& name = "scene");

			/*
			 * Synchronous serializing
			 */
			void serialize(std::ostream& stream, StorageFormat fmt, const std::string& basePath);
			void serialize(const std::string& filename, StorageFormat fmt);

			/*
			 * Synchronous deserializing
			 */
			bool deserialize(std::istream& stream, RenderContext* rc, ObjectsCache& tc,
				ElementFactory* factory, const std::string& basePath);
			bool deserialize(const std::string& filename, RenderContext* rc, ObjectsCache& tc,
				ElementFactory* factory);

			/*
			 * Asynchronous deserializing
			 */
			void deserializeAsync(std::istream& stream, RenderContext* rc, ObjectsCache& tc,
				ElementFactory* factory, const std::string& basePath);
			void deserializeAsync(const std::string& filename, RenderContext* rc, ObjectsCache& tc,
				ElementFactory* factory);

			/*
			 * Access to content
			 */
			VertexBuffer::Pointer vertexBufferWithId(const std::string& id);
			
			IndexBuffer indexBufferWithId(const std::string& id);
			
			VertexArrayObject vaoWithIdentifiers(const std::string& vbid, const std::string& ibid);
			
		public:
			ET_DECLARE_EVENT1(deserializationFinished, bool)

		private:
			bool performDeserialization(std::istream& stream, RenderContext* rc, ObjectsCache& tc,
				ElementFactory* factory, const std::string& basePath, bool async);

			void buildAPIObjects(Scene3dStorage::Pointer p, RenderContext* rc);

			Scene3dStorage::Pointer deserializeStorage(std::istream& stream, RenderContext* rc,
				ObjectsCache& tc, const std::string& basePath, StorageFormat, StorageVersion, bool async);
			
			Element::Pointer createElementOfType(size_t type, Element* parent);
			Material::Pointer materialWithId(uint64_t);

			void onMaterialLoaded(Material*);
			void allMaterialsLoaded();

		private:
			ElementFactory* _externalFactory;
			std::vector<VertexBuffer::Pointer> _vertexBuffers;
			std::vector<IndexBuffer> _indexBuffers;
			std::vector<VertexArrayObject> _vaos;
			AtomicCounter _materialsToLoad;
			AtomicCounter _componentsToLoad;
		};
	}
}
