/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/storage.h>
#include <et/scene3d/serialization.h>

using namespace et;
using namespace et::s3d;

Scene3dStorage::Scene3dStorage(const std::string& name, Element* parent) : 
	ElementContainer(name, parent)
{
	_indexArray = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, 0, PrimitiveType::Triangles);
}

void Scene3dStorage::addVertexStorage(const VertexStorage::Pointer& vs)
{
	_vertexStorages.push_back(vs);
}

void Scene3dStorage::setIndexArray(const IndexArray::Pointer& ia)
{
	_indexArray = ia;
}

VertexStorage::Pointer Scene3dStorage::addVertexStorageWithDeclaration(const VertexDeclaration& decl, size_t size)
{
	_vertexStorages.push_back(VertexStorage::Pointer::create(decl, size));
	return _vertexStorages.back();
}

VertexStorage::Pointer Scene3dStorage::vertexStorageWithDeclarationForAppendingSize(const VertexDeclaration& decl, size_t size)
{
	for (const auto&  i : _vertexStorages)
	{
		if ((i->declaration() == decl) && (i->capacity() + size < IndexArray::MaxShortIndex))
			return i;
	}
	
	return addVertexStorageWithDeclaration(decl, 0);
	
}

int Scene3dStorage::indexOfVertexStorage(const VertexStorage::Pointer& vs)
{
	int index = 0;

	for (const auto&  i : _vertexStorages)
	{
		if (i == vs)
			return index;
		
		++index;
	}

	return -1;
}

void Scene3dStorage::serialize(std::ostream& stream, SceneVersion)
{
	serializeUInt32(stream, 0);
}

void Scene3dStorage::deserialize(std::istream& stream, ElementFactory*, SceneVersion)
{
	uint32_t value = deserializeUInt32(stream);
	ET_ASSERT(value == 0);
	(void)(value);
}

void Scene3dStorage::flush()
{
	auto vi = _vertexStorages.begin();
	while (vi != _vertexStorages.end())
	{
		if (vi->ptr()->atomicCounterValue() == 1)
			vi = _vertexStorages.erase(vi);
		else
			++vi;
	}
	
	Material::List::iterator mi = _materials.begin();
	while (mi != _materials.end())
	{
		if (mi->ptr()->atomicCounterValue() == 1)
			mi = _materials.erase(mi);
		else
			++mi;
	}
	
	auto ti = _textures.begin();
	while (ti != _textures.end())
	{
		if (ti->ptr()->atomicCounterValue() == 1)
			ti = _textures.erase(ti);
		else
			++ti;
	}
}
