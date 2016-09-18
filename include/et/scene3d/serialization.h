/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>
#include <et/scene3d/base.h>

namespace et
{
	class VertexStream;
	class VertexStorage;
	class IndexArray;
	class Material;
	
	namespace s3d
	{
		class BaseElement;
		class SceneMaterial;
		class SerializationHelper
		{
		public:
			virtual const std::string& serializationBasePath() const = 0;
			virtual IntrusivePtr<BaseElement> createElementOfType(ElementType, BaseElement*) = 0;
			
			virtual IntrusivePtr<VertexStorage> vertexStorageWithName(const std::string&) = 0;
			virtual IntrusivePtr<IndexArray> indexArrayWithName(const std::string&) = 0;
			
			virtual IntrusivePtr<SceneMaterial> sceneMaterialWithName(const std::string&) = 0;
			
			virtual IntrusivePtr<VertexStream> vertexStreamWithStorageName(const std::string&) = 0;
			virtual IntrusivePtr<Material> materialWithName(const std::string&) = 0;

		public:
			virtual ~SerializationHelper() { }
		};

		enum : uint32_t
		{
			DeserializeOption_LoadHierarchy = 0x0000,
			
			DeserializeOption_KeepMeshes = 0x0001,
			
			DeserializeOption_CreateTextures = 0x0010,
			DeserializeOption_CreateVertexBuffers = 0x0020,
			
			DeserializeOption_KeepAndCreateEverything = DeserializeOption_LoadHierarchy |
				DeserializeOption_KeepMeshes |
				DeserializeOption_CreateTextures | DeserializeOption_CreateVertexBuffers
		};
	}
}
