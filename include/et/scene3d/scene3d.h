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
			void deserialize(et::RenderContext*, Dictionary, const std::string& basePath, ObjectsCache&);

			Storage& storage()
				{ return _storage; }

			const Storage& storage() const
				{ return _storage; }

		public:
			ET_DECLARE_EVENT1(deserializationFinished, bool)

		private:
			Material* materialWithName(const std::string&);

			BaseElement::Pointer createElementOfType(ElementType, BaseElement*);

			const std::string& serializationBasePath() const 
				{ return _serializationBasePath; }

		private:
			Storage _storage;
			std::string _serializationBasePath;
		};
	}
}
