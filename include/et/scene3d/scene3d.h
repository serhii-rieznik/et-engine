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
		class Scene : public ElementContainer, public ElementFactory
		{
		public:
			ET_DECLARE_POINTER(Scene)
			
		public:
			Scene(const std::string& name = "scene");

			Dictionary serialize(const std::string&);

		public:
			ET_DECLARE_EVENT1(deserializationFinished, bool)

		private:
			BaseElement::Pointer createElementOfType(uint64_t type, BaseElement* parent);
			Material::Pointer materialWithName(const std::string&);
			IndexArray::Pointer primaryIndexArray();
			VertexStorage::Pointer vertexStorageWithName(const std::string&);
		};
	}
}
