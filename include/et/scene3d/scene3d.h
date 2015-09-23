/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/events.h>
#include <et/core/objectscache.h>
#include <et/scene3d/elementcontainer.h>
#include <et/scene3d/storage.h>
#include <et/scene3d/supportmesh.h>
#include <et/scene3d/cameraelement.h>
#include <et/scene3d/lightelement.h>
#include <et/scene3d/particlesystem.h>
#include <et/scene3d/lineelement.h>
#include <et/scene3d/skeletonelement.h>

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
			
			void deserialize(et::RenderContext*, Dictionary, const std::string& basePath, ObjectsCache&,
				bool shouldCreateRenderObjects = true);

			Storage& storage()
				{ return _storage; }

			const Storage& storage() const
				{ return _storage; }

		public:
			ET_DECLARE_EVENT1(deserializationFinished, bool)

		private:
			void buildVertexBuffers(et::RenderContext*);
			
			Material* materialWithName(const std::string&) override;

			BaseElement::Pointer createElementOfType(ElementType, BaseElement*) override;

			const std::string& serializationBasePath() const override
				{ return _serializationBasePath; }

			IndexArray::Pointer indexArrayWithName(const std::string&) override;
			VertexStorage::Pointer vertexStorageWithName(const std::string&) override;
			
			VertexArrayObject vertexArrayWithStorageName(const std::string&) override;
						
		private:
			Storage _storage;
			std::string _serializationBasePath;
			std::vector<VertexArrayObject> _vertexArrays;
			IndexBuffer::Pointer _mainIndexBuffer;
			bool _shouldCreateRenderObjects = true;
		};
	}
}
