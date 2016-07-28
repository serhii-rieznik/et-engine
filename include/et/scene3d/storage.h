/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene3d/scenematerial.h>
#include <et/rendering/vertexarrayobject.h>
#include <et/rendering/base/indexarray.h>
#include <et/rendering/base/vertexstorage.h>

namespace et
{
	namespace s3d
	{
		class Storage
		{
		public:
			Storage();

			Dictionary serialize(const std::string&);
			
			void deserializeWithOptions(RenderContext*, Dictionary, SerializationHelper*, ObjectsCache&,
				uint32_t);

			Set<VertexStorage::Pointer>& vertexStorages()
				{ return _vertexStorages; }

			const Set<VertexStorage::Pointer>& vertexStorages() const
				{ return _vertexStorages; }
			
			IndexArray::Pointer indexArray()
				{ return _indexArray; }

			const IndexArray::Pointer indexArray() const
				{ return _indexArray; }

			SceneMaterial::Map& materials()
				{ return _materials; }

			const SceneMaterial::Map& materials() const
				{ return _materials; }

			Set<Texture::Pointer>& textures()
				{ return _textures; }

			const Set<Texture::Pointer>& textures() const
				{ return _textures; }
            
            const Set<VertexArrayObject::Pointer>& vertexArrayObjects() const
                { return _vertexArrayObjects; }
            
			void addTexture(Texture::Pointer t)
				{ _textures.insert(t); }

			void addMaterial(SceneMaterial::Pointer m)
				{ _materials.insert({m->name(), m}); }

			void addVertexStorage(const VertexStorage::Pointer&);

			void setIndexArray(const IndexArray::Pointer&);
			
			VertexStorage::Pointer addVertexStorageWithDeclaration(const VertexDeclaration& decl, size_t size);
			VertexStorage::Pointer vertexStorageWithDeclarationForAppendingSize(const VertexDeclaration& decl, size_t size);
			
			int indexOfVertexStorage(const VertexStorage::Pointer& va);
			
			void flush();
            
            void buildVertexArrayObjects(RenderContext* rc);

		private:
			Storage* duplicate()
				{ return nullptr; }

		private:
			Set<VertexStorage::Pointer> _vertexStorages;
            Set<Texture::Pointer> _textures;
            Set<VertexArrayObject::Pointer> _vertexArrayObjects;
			IndexArray::Pointer _indexArray;
			SceneMaterial::Map _materials;
		};
	}
}
