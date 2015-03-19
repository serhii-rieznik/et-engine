/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/serialization.h>
#include <et/vertexbuffer/vertexarray.h>

using namespace et;

const uint32_t VertexArrayId_1 = ET_COMPOSE_UINT32('V', 'A', 'V', '1');
const uint32_t VertexArrayId_2 = ET_COMPOSE_UINT32('V', 'A', 'V', '2');
const uint32_t VertexArrayCurrentId = VertexArrayId_2;

VertexArray::VertexArray() : tag(0), _decl(true), _size(0),
	_smoothing(VertexAttributeUsage::Smoothing, VertexAttributeType::Int, 0) { }

VertexArray::VertexArray(const VertexDeclaration& decl, size_t size) : tag(0), _decl(decl.interleaved()),
	_size(size), _smoothing(VertexAttributeUsage::Smoothing, VertexAttributeType::Int, size)
{
	for (size_t i = 0; i < decl.numElements(); ++i)
	{
		const VertexElement& e = decl.element(i);
		_decl.push_back(e.usage(), e.type());
		_chunks.push_back(VertexDataChunk(e.usage(), e.type(), size));
	}
}

VertexArray::VertexArray(const VertexDeclaration& decl, int size) : tag(0), _decl(decl.interleaved()),
	_size(static_cast<size_t>(size)), _smoothing(VertexAttributeUsage::Smoothing, VertexAttributeType::Int, _size)
{
	for (size_t i = 0; i < decl.numElements(); ++i)
	{
		const VertexElement& e = decl.element(i);
		_decl.push_back(e.usage(), e.type());
		_chunks.push_back(VertexDataChunk(e.usage(), e.type(), _size));
	}
}

VertexArray::Description VertexArray::generateDescription() const
{
	size_t dataSize = 0;
	size_t offset = 0;

	Description desc;
	desc.declaration = VertexDeclaration(_decl.interleaved());

	for (auto& chunk : _chunks)
	{
		uint32_t t_stride = static_cast<uint32_t>(_decl.interleaved() ? _decl.dataSize() : 0);
		uint32_t t_offset = static_cast<uint32_t>(_decl.interleaved() ? offset : dataSize);
		desc.declaration.push_back(VertexElement(chunk->usage(), chunk->type(), t_stride, t_offset));
		dataSize += chunk->dataSize();
		offset += chunk->typeSize();
	}

	desc.data.resize(dataSize);
	size_t numElements = dataSize / desc.declaration.dataSize();
	char* ptr0 = desc.data.binary();

	size_t entry_i = 0;
	for (auto& chunk : _chunks)
	{
		const char* chunkData = chunk->data();
		size_t chunkDataSize = chunk->dataSize();
		size_t chunkOffset = desc.declaration[entry_i++].offset();
		if (desc.declaration.interleaved())
		{
			for (size_t j = 0; j < numElements; ++j)
			{
				size_t dstPtrOffset = chunkOffset + j * offset;
				ET_ASSERT(dstPtrOffset < desc.data.dataSize());
				char* dstPtr = ptr0 + dstPtrOffset;
				size_t srcPtrOffset = j * chunk->typeSize();
				ET_ASSERT(srcPtrOffset < chunkDataSize);
				const char* srcPtr = chunkData + srcPtrOffset;
				etCopyMemory(dstPtr, srcPtr, chunk->typeSize());
			}
		}
		else
		{
			etCopyMemory(ptr0 + chunkOffset, chunkData, chunkDataSize);
		}
	}

	return desc;
}

const VertexDataChunk VertexArray::chunk(VertexAttributeUsage usage) const
{
	for (auto& i : _chunks)
	{
		if (i->usage() == usage)
			return i;
	}
	
	return VertexDataChunk();
}

VertexDataChunk VertexArray::chunk(VertexAttributeUsage usage)
{
	for (auto& i : _chunks)
	{
		if (i->usage() == usage)
			return i;
	}
	
	return VertexDataChunk();
}

void VertexArray::resize(size_t size)
{
	_size = size;
	
	for (auto& i :_chunks)
		i->resize(_size);
	
	_smoothing->resize(_size);
}

void VertexArray::increase(size_t count)
{
	resize(_size + count);
}

void VertexArray::fitToSize(size_t count)
{
	if (_size < count)
		resize(count);
}

VertexArray* VertexArray::duplicate()
{
	VertexArray* result = sharedObjectFactory().createObject<VertexArray>(_decl, _size);
	
	for (auto& c : _chunks)
		c->copyTo(result->chunk(c->usage()).reference());

	return result;
}
