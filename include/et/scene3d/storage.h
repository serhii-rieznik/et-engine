/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/Material.h>
#include <et/rendering/base/indexarray.h>
#include <et/rendering/base/vertexstorage.h>
#include <et/rendering/base/vertexstream.h>
#include <et/rendering/interface/texture.h>

namespace et
{
namespace s3d
{
class Storage
{
public:
	Storage();

	Set<VertexStorage::Pointer>& vertexStorages()
		{ return _vertexStorages; }

	const Set<VertexStorage::Pointer>& vertexStorages() const
		{ return _vertexStorages; }
	
	IndexArray::Pointer indexArray()
		{ return _indexArray; }

	const IndexArray::Pointer indexArray() const
		{ return _indexArray; }

	MaterialInstance::Map& materials()
		{ return _materials; }

	const MaterialInstance::Map& materials() const
		{ return _materials; }

	Set<Texture::Pointer>& textures()
		{ return _textures; }

	const Set<Texture::Pointer>& textures() const
		{ return _textures; }
	
	const Set<VertexStream::Pointer>& vertexStreams() const
		{ return _vertexStreams; }
	
	void addTexture(Texture::Pointer t)
		{ _textures.insert(t); }

	void addMaterial(MaterialInstance::Pointer m)
		{ _materials.insert({m->name(), m}); }

	void addVertexStorage(const VertexStorage::Pointer&);

	void setIndexArray(const IndexArray::Pointer&);
	
	VertexStorage::Pointer addVertexStorageWithDeclaration(const VertexDeclaration& decl, uint32_t size);
	VertexStorage::Pointer vertexStorageWithDeclarationForAppendingSize(const VertexDeclaration& decl, uint32_t size);
	
	int indexOfVertexStorage(const VertexStorage::Pointer& va);
	
	void flush();
	
	void buildVertexStreams(RenderContext* rc);

private:
	Storage* duplicate()
		{ return nullptr; }

private:
	Set<VertexStorage::Pointer> _vertexStorages;
	Set<Texture::Pointer> _textures;
	Set<VertexStream::Pointer> _vertexStreams;
	IndexArray::Pointer _indexArray;
	MaterialInstance::Map _materials;
};
}
}
