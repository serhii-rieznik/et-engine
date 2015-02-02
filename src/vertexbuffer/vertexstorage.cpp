/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/datastorage.h>
#include <et/vertexbuffer/vertexstorage.h>

using namespace et;

class et::VertexStoragePrivate
{
public:
	VertexStoragePrivate(const VertexDeclaration&, size_t);

public:
	VertexDeclaration decl;
	BinaryDataStorage data;
	size_t capacity = 0;
};

VertexStorage::VertexStorage(const VertexDeclaration& aDecl, size_t capacity)
{
	ET_PIMPL_INIT(VertexStorage, aDecl, capacity);
}

VertexStorage::~VertexStorage()
{
	ET_PIMPL_FINALIZE(VertexStorage)
}

bool VertexStorage::hasAttribute(VertexAttributeUsage usage) const
{
	return declaration().has(usage);
}

bool VertexStorage::hasAttributeWithType(VertexAttributeUsage usage, VertexAttributeType type) const
{
	return hasAttribute(usage) && (_private->decl.elementForUsage(usage).type() == type);
}

BinaryDataStorage& VertexStorage::data()
{
	return _private->data;
}

const BinaryDataStorage& VertexStorage::data() const
{
	return _private->data;
}

size_t VertexStorage::offsetOfAttribute(VertexAttributeUsage usage) const
{
	ET_ASSERT(hasAttribute(usage));
	return declaration().elementForUsage(usage).offset();
}

size_t VertexStorage::sizeOfAttribute(VertexAttributeUsage usage) const
{
	ET_ASSERT(hasAttribute(usage));
	const auto& element = declaration().elementForUsage(usage);
	return sizeOfDataType(element.dataType()) * vertexAttributeTypeComponents(element.type());
}

size_t VertexStorage::stride() const
{
	return declaration().dataSize();
}

const VertexDeclaration& VertexStorage::declaration() const
{
	return _private->decl;
}

/*
 * Private
 */
VertexStoragePrivate::VertexStoragePrivate(const VertexDeclaration& d, size_t cap) :
	decl(d), capacity(cap), data(d.dataSize() * cap, 0)
{

}