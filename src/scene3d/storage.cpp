/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/scene3d/storage.h>
#include <et/scene3d/serialization.h>

using namespace et;
using namespace et::s3d;

Scene3dStorage::Scene3dStorage(const std::string& name, Element* parent) : 
	ElementContainer(name, parent), _indexArray(new IndexArray(IndexArrayFormat_16bit, 0, PrimitiveType_Triangles))
{
}

void Scene3dStorage::addVertexArray(const VertexArray::Pointer& va)
{
	_vertexArrays.push_back(va);
}

VertexArray::Pointer Scene3dStorage::addVertexArrayWithDeclaration(const VertexDeclaration& decl, size_t size)
{
	_vertexArrays.push_back(VertexArray::Pointer::create(decl, size));
	return _vertexArrays.back();
}

VertexArray::Pointer Scene3dStorage::vertexArrayWithDeclaration(const VertexDeclaration& decl)
{
	for (VertexArrayList::iterator i = _vertexArrays.begin(), e = _vertexArrays.end(); i != e; ++i)
	{
		if (((*i)->decl() == decl))
			return *i;
	}

	return addVertexArrayWithDeclaration(decl, 0);
}

VertexArray::Pointer Scene3dStorage::vertexArrayWithDeclarationForAppendingSize(const VertexDeclaration& decl, size_t size)
{
	for (VertexArrayList::iterator i = _vertexArrays.begin(), e = _vertexArrays.end(); i != e; ++i)
	{
		if (((*i)->decl() == decl) && ((*i)->size() + size < IndexArray::MaxShortIndex))
			return *i;
	}

	return addVertexArrayWithDeclaration(decl, 0);
}

int Scene3dStorage::indexOfVertexArray(const VertexArray::Pointer& va)
{
	int index = 0;

	for (VertexArrayList::iterator i = _vertexArrays.begin(), e = _vertexArrays.end(); i != e; ++i, ++index)
	{
		if (*i == va)
			return index;
	}

	return -1;
}

void Scene3dStorage::serialize(std::ostream& stream, SceneVersion)
{
	serializeInt(stream, 0);
}

void Scene3dStorage::deserialize(std::istream& stream, ElementFactory*, SceneVersion)
{
	int value = deserializeInt(stream);
	ET_ASSERT(value == 0);
	(void)(value);
}

void Scene3dStorage::flush()
{
	VertexArrayList::iterator vi = _vertexArrays.begin();
	while (vi != _vertexArrays.end())
	{
		VertexArray* ptr = vi->ptr();
		if (ptr->atomicCounterValue() == 1)
		{
			vi = _vertexArrays.erase(vi);
		}
		else
		{
			++vi;
		}
	}
	
	Material::List::iterator mi = _materials.begin();
	while (mi != _materials.end())
	{
		Material* ptr = mi->ptr();
		if (ptr->atomicCounterValue() == 1)
		{
			mi = _materials.erase(mi);
		}
		else
		{
			++mi;
		}
	}
	
	TextureList::iterator ti = _textures.begin();
	while (ti != _textures.end())
	{
		TextureData* ptr = ti->ptr();
		if (ptr->atomicCounterValue() == 1)
		{
			ti = _textures.erase(ti);
		}
		else
		{
			++ti;
		}
	}
}