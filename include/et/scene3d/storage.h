/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/vertexbuffer/indexarray.h>
#include <et/vertexbuffer/vertexstorage.h>
#include <et/scene3d/material.h>
#include <et/scene3d/baseelement.h>

namespace et
{
	namespace s3d
	{
		class Scene3dStorage : public ElementContainer
		{
		public:
			ET_DECLARE_POINTER(Scene3dStorage)

		public:
			Scene3dStorage(const std::string& name, Element* parent);

			void serialize(std::ostream& stream, SceneVersion version);
			void deserialize(std::istream& stream, ElementFactory* factory, SceneVersion version);

			ElementType type() const
				{ return ElementType_Storage; }

			std::vector<VertexStorage::Pointer>& vertexStorages()
				{ return _vertexStorages; }
			const std::vector<VertexStorage::Pointer>& vertexStorages() const
				{ return _vertexStorages; }
			
			IndexArray::Pointer indexArray()
				{ return _indexArray; }
			const IndexArray::Pointer indexArray() const
				{ return _indexArray; }

			Material::List& materials()
				{ return _materials; }
			const Material::List& materials() const
				{ return _materials; }

			std::vector<Texture::Pointer>& textures()
				{ return _textures; }
			const std::vector<Texture::Pointer>& textures() const
				{ return _textures; }

			void addTexture(Texture::Pointer t)
				{ _textures.push_back(t); }

			void addMaterial(Material::Pointer m)
				{ _materials.push_back(m); }

			void addVertexStorage(const VertexStorage::Pointer&);
			void setIndexArray(const IndexArray::Pointer&);
			
			VertexStorage::Pointer addVertexStorageWithDeclaration(const VertexDeclaration& decl, size_t size);
			VertexStorage::Pointer vertexStorageWithDeclarationForAppendingSize(const VertexDeclaration& decl, size_t size);
			
			int indexOfVertexStorage(const VertexStorage::Pointer& va);
			
			void flush();

		private:
			Scene3dStorage* duplicate()
				{ return 0; }

		private:
			std::vector<VertexStorage::Pointer> _vertexStorages;
			IndexArray::Pointer _indexArray;
			Material::List _materials;
			std::vector<Texture::Pointer> _textures;
		};
	}
}
