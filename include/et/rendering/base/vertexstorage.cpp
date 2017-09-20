/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/datastorage.h>
#include <et/rendering/base/vertexstorage.h>

namespace et
{

class VertexStoragePrivate
{
public:
	VertexStoragePrivate(const VertexDeclaration&, uint32_t);

public:
	VertexDeclaration decl;
	BinaryDataStorage data;
	uint32_t capacity = 0;
};

VertexStorage::VertexStorage(const VertexDeclaration& aDecl, uint32_t capacity)
{
	ET_PIMPL_INIT(VertexStorage, aDecl, capacity);
}

VertexStorage::VertexStorage(const VertexArray::Pointer& va)
{
	auto desc = va->generateDescription();
	ET_ASSERT(desc.declaration.interleaved());
	ET_PIMPL_INIT(VertexStorage, desc.declaration, va->size());
	memcpy(_private->data.binary(), desc.data.binary(), desc.data.dataSize());
}

VertexStorage::~VertexStorage()
{
	ET_PIMPL_FINALIZE(VertexStorage);
}

bool VertexStorage::hasAttribute(VertexAttributeUsage usage) const
{
	return declaration().has(usage);
}

bool VertexStorage::hasAttributeWithType(VertexAttributeUsage usage, DataType type) const
{
	return hasAttribute(usage) && (_private->decl.elementForUsage(usage).type() == type);
}

DataType VertexStorage::attributeType(VertexAttributeUsage usage) const
{
	ET_ASSERT(hasAttribute(usage));
	return _private->decl.elementForUsage(usage).type();
}

BinaryDataStorage& VertexStorage::data()
{
	return _private->data;
}

const BinaryDataStorage& VertexStorage::data() const
{
	return _private->data;
}

uint32_t VertexStorage::offsetOfAttribute(VertexAttributeUsage usage) const
{
	ET_ASSERT(hasAttribute(usage));
	return declaration().elementForUsage(usage).offset();
}

uint32_t VertexStorage::sizeOfAttribute(VertexAttributeUsage usage) const
{
	ET_ASSERT(hasAttribute(usage));
	const auto& element = declaration().elementForUsage(usage);
	return sizeOfDataFormat(element.dataFormat()) * dataTypeComponents(element.type());
}

uint32_t VertexStorage::stride() const
{
	return declaration().sizeInBytes();
}

const VertexDeclaration& VertexStorage::declaration() const
{
	return _private->decl;
}

uint32_t VertexStorage::capacity() const
{
	return _private->capacity;
}

void VertexStorage::increaseSize(uint32_t sz)
{
	resize(_private->capacity + sz);
}

void VertexStorage::resize(uint32_t sz)
{
	_private->capacity = sz;
	_private->data.resize(_private->capacity * _private->decl.sizeInBytes());
}

void VertexStorage::serialize(std::ostream& fOut)
{
	serializeUInt32(fOut, 0);
	_private->decl.serialize(fOut);
	
	serializeUInt32(fOut, _private->data.size());
	if (_private->data.size() > 0)
		fOut.write(_private->data.binary(), _private->data.size());
}

void VertexStorage::deserialize(std::istream& fIn)
{
	uint32_t id = deserializeUInt32(fIn);
	_private->decl.deserialize(fIn);
	
	uint32_t dataSize = deserializeUInt32(fIn);
	_private->data.resize(dataSize);
	if (dataSize > 0)
		fIn.read(_private->data.binary(), dataSize);
}

/*
 * Private
 */
VertexStoragePrivate::VertexStoragePrivate(const VertexDeclaration& d, uint32_t cap) :
	decl(d), data(d.sizeInBytes() * cap, 0), capacity(cap)
{
}

}
