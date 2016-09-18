/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/serialization.h>
#include <et/rendering/base/vertexdatachunk.h>

namespace et
{

VertexDataChunkData::VertexDataChunkData(VertexAttributeUsage aUsage, DataType aType, uint32_t aSize) :
	_usage(aUsage), _type(aType)
{
	ET_ASSERT(_type < DataType::max);
	
	_data.resize(dataTypeSize(_type) * aSize);
	_data.fill(0);
}

VertexDataChunkData::VertexDataChunkData(std::istream& stream)
{
	_usage = static_cast<VertexAttributeUsage>(deserializeUInt32(stream));
	
	_type = static_cast<DataType>(deserializeUInt32(stream));
	ET_ASSERT(_type < DataType::max);

	uint32_t capacity = deserializeUInt32(stream);
	uint32_t offset = deserializeUInt32(stream);
	
	_data.resize(capacity);
	_data.setOffset(offset);
	
	stream.read(_data.binary(), capacity);
}

void VertexDataChunkData::resize(uint32_t sz)
{
	_data.resize(typeSize() * sz);
}

void VertexDataChunkData::fitToSize(uint32_t count)
{
	if (_data.dataSize() < typeSize() * count)
		resize(count);
}

void VertexDataChunkData::copyTo(VertexDataChunkData& c) const
{
	c._usage = _usage;
	c._type = _type;
	c._data = _data;
}

}