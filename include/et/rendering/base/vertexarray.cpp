/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/serialization.h>
#include <et/rendering/base/vertexarray.h>

namespace et
{

VertexArray::VertexArray(const VertexDeclaration& decl, uint32_t size) : _size(size),
	_decl(decl.interleaved())
{
	for (uint32_t i = 0; i < decl.numElements(); ++i)
	{
		const VertexElement& e = decl.element(i);
		_decl.push_back(e.usage(), e.type());
		_chunks.push_back(VertexDataChunk::create(e.usage(), e.type(), size));
	}
}

VertexArray::Description VertexArray::generateDescription() const
{
	uint32_t dataSize = 0;
	uint32_t offset = 0;

	Description desc;
	desc.declaration = VertexDeclaration(_decl.interleaved());

	for (auto& chunk : _chunks)
	{
		uint32_t t_stride = static_cast<uint32_t>(_decl.interleaved() ? _decl.sizeInBytes() : 0);
		uint32_t t_offset = static_cast<uint32_t>(_decl.interleaved() ? offset : dataSize);
		desc.declaration.push_back(VertexElement(chunk->usage(), chunk->type(), t_stride, t_offset));
		dataSize += chunk->dataSize();
		offset += chunk->typeSize();
	}

	desc.data.resize(dataSize);
	uint32_t numElements = dataSize / desc.declaration.sizeInBytes();
	char* ptr0 = desc.data.binary();

	uint32_t entry_i = 0;
	for (auto& chunk : _chunks)
	{
		const char* chunkData = chunk->data();
		uint32_t chunkDataSize = static_cast<uint32_t>(chunk->dataSize());
		uint32_t chunkOffset = desc.declaration[entry_i++].offset();
		if (desc.declaration.interleaved())
		{
			for (uint32_t j = 0; j < numElements; ++j)
			{
				uint32_t dstPtrOffset = chunkOffset + j * offset;
				ET_ASSERT(dstPtrOffset < desc.data.dataSize());
				char* dstPtr = ptr0 + dstPtrOffset;
				uint32_t srcPtrOffset = static_cast<uint32_t>(j * chunk->typeSize());
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

void VertexArray::resize(uint32_t size)
{
	_size = size;
	for (auto& i :_chunks)
		i->resize(_size);
}

void VertexArray::increase(uint32_t count)
{
	resize(_size + count);
}

void VertexArray::fitToSize(uint32_t count)
{
	if (_size < count)
		resize(count);
}

VertexArray* VertexArray::duplicate()
{
	VertexArray* result = etCreateObject<VertexArray>(_decl, _size);
	
	for (auto& c : _chunks)
		c->copyTo(result->chunk(c->usage()).reference());

	return result;
}

}
