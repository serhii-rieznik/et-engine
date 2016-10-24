/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/json.h>
#include <et/rendering/rendercontext.h>
#include <et/scene3d/storage.h>
#include <et/scene3d/serialization.h>

using namespace et;
using namespace et::s3d;

Storage::Storage()
{
}

void Storage::addVertexStorage(const VertexStorage::Pointer& vs)
{
	_vertexStorages.insert(vs);
}

void Storage::setIndexArray(const IndexArray::Pointer& ia)
{
	_indexArray = ia;
}

VertexStorage::Pointer Storage::addVertexStorageWithDeclaration(const VertexDeclaration& decl, uint32_t size)
{
	auto storage = VertexStorage::Pointer::create(decl, size);
	storage->setName("vertexStorage" + intToStr(_vertexStorages.size()));
	_vertexStorages.insert(storage);
    return storage;
}

VertexStorage::Pointer Storage::vertexStorageWithDeclarationForAppendingSize(const VertexDeclaration& decl, uint32_t size)
{
	for (const auto&  i : _vertexStorages)
	{
		if (i->declaration().hasSameElementsAs(decl) && (i->capacity() + size < IndexArray::MaxShortIndex))
			return i;
	}
	
	return addVertexStorageWithDeclaration(decl, 0);
}

int Storage::indexOfVertexStorage(const VertexStorage::Pointer& vs)
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

void Storage::flush()
{
	auto vi = _vertexStorages.begin();
	while (vi != _vertexStorages.end())
	{
		if (vi->ptr()->retainCount() == 1)
			vi = _vertexStorages.erase(vi);
		else
			++vi;
	}
	
	if (_indexArray.valid() && (_indexArray->retainCount() == 1))
		_indexArray = IndexArray::Pointer();
	
	MaterialInstance::Map::iterator mi = _materials.begin();
	while (mi != _materials.end())
	{
		if (mi->second.ptr()->retainCount() == 1)
			mi = _materials.erase(mi);
		else
			++mi;
	}
	
	auto ti = _textures.begin();
	while (ti != _textures.end())
	{
		if (ti->ptr()->retainCount() == 1)
			ti = _textures.erase(ti);
		else
			++ti;
	}
}

void Storage::buildVertexArrayObjects(RenderContext* rc)
{
    IndexBuffer::Pointer ib;
    for (auto vs : _vertexStorages)
    {
        std::string vaoName = "vao-" + intToStr(_vertexStreams.size() + 1);
        if (ib.invalid())
        {
            ib = rc->renderer()->createIndexBuffer("mainIndexBuffer", _indexArray, BufferDrawType::Static);
        }
        auto vb = rc->renderer()->createVertexBuffer(vs->name(), vs, BufferDrawType::Static);
        _vertexStreams.insert(VertexStream::Pointer::create(vb, ib));
    }
}
