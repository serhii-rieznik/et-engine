/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/serialization.h>
#include <et/vertexbuffer/vertexdatachunk.h>

namespace et
{
	VertexAttributeType openglTypeToVertexAttributeType(uint32_t);
}

using namespace et;

VertexDataChunkData::VertexDataChunkData(VertexAttributeUsage aUsage, VertexAttributeType aType, size_t aSize) :
	_usage(aUsage), _type(aType)
{
	ET_ASSERT(_type < VertexAttributeType::max);
	
	_data.resize(vertexAttributeTypeSize(_type) * aSize);
	_data.fill(0);
}

void VertexDataChunkData::serialize(std::ostream& stream)
{
	serializeUInt32(stream, static_cast<uint32_t>(_usage));
	serializeUInt32(stream, static_cast<uint32_t>(_type));
	serializeUInt32(stream, static_cast<uint32_t>(_data.dataSize()));
	serializeUInt32(stream, static_cast<uint32_t>(_data.lastElementIndex()));
	stream.write(_data.binary(), _data.dataSize());
}

VertexDataChunkData::VertexDataChunkData(std::istream& stream)
{
	_usage = static_cast<VertexAttributeUsage>(deserializeUInt32(stream));
	_type = static_cast<VertexAttributeType>(deserializeUInt32(stream));
	
	uint32_t capacity = deserializeUInt32(stream);
	uint32_t offset = deserializeUInt32(stream);
	
	_data.resize(capacity);
	_data.setOffset(offset);
	
	stream.read(_data.binary(), capacity);
		
	/*
	 * Support legacy values
	 */
	if (_type >= VertexAttributeType::max)
		_type = openglTypeToVertexAttributeType(static_cast<uint32_t>(_type));
}

void VertexDataChunkData::resize(size_t sz)
{
	_data.resize(typeSize() * sz);
}

void VertexDataChunkData::fitToSize(size_t count)
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
