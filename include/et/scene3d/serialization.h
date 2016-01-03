/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <ostream>
#include <istream>
#include <et/core/et.h>
#include <et/scene3d/base.h>
#include <et/vertexbuffer/vertexstorage.h>
#include <et/vertexbuffer/indexarray.h>
#include <et/rendering/vertexarrayobject.h>

namespace et
{
	namespace s3d
	{
		class SceneMaterial;
		class BaseElement;
		typedef IntrusivePtr<BaseElement> BaseElementPointer;

		class SerializationHelper
		{
		public:
			virtual const std::string& serializationBasePath() const = 0;
			virtual BaseElementPointer createElementOfType(ElementType, BaseElement*) = 0;
			virtual SceneMaterial* materialWithName(const std::string&) = 0;
			
			virtual IndexArray::Pointer indexArrayWithName(const std::string&) = 0;
			virtual VertexStorage::Pointer vertexStorageWithName(const std::string&) = 0;
			
			virtual VertexArrayObject vertexArrayWithStorageName(const std::string&) = 0;

		public:
			virtual ~SerializationHelper() { }
		};

		enum : uint32_t
		{
			DeserializeOption_LoadHierarchy = 0x0000,
			
			DeserializeOption_KeepGeometry = 0x0001,
			DeserializeOption_KeepSupportMeshes = 0x0002,
			
			DeserializeOption_CreateTextures = 0x0010,
			DeserializeOption_CreateVertexBuffers = 0x0020,
			
			DeserializeOption_KeepAndCreateEverything = DeserializeOption_LoadHierarchy |
				DeserializeOption_KeepGeometry | DeserializeOption_KeepSupportMeshes | 
				DeserializeOption_CreateTextures | DeserializeOption_CreateVertexBuffers
		};
	}
}
