/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/serialization.h>
#include <et/vertexbuffer/vertexdatachunk.h>

using namespace et;

VertexDataChunkData::VertexDataChunkData(VertexAttributeUsage usage, VertexAttributeType type, size_t size) : _usage(usage), 
	_type(type), _data(vertexAttributeTypeSize(type) * size)
{

}

VertexDataChunkData::VertexDataChunkData(std::istream& stream)
{
	_usage = static_cast<VertexAttributeUsage>(deserializeInt(stream));
	_type = static_cast<VertexAttributeType>(deserializeInt(stream));
	_data.resize(deserializeUInt(stream));
	_data.setOffset(deserializeUInt(stream));
	stream.read(_data.binary(), static_cast<std::streamsize>(_data.dataSize()));
}

void VertexDataChunkData::serialize(std::ostream& stream)
{
	serializeInt(stream, _usage);
	serializeInt(stream, _type);
	serializeInt(stream, static_cast<int>(_data.dataSize()));
	serializeInt(stream, _data.lastElementIndex());
	stream.write(_data.binary(), static_cast<std::streamsize>(_data.dataSize()));
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

void VertexDataChunkData::copyTo(VertexDataChunkData& c)
{
	c._usage = _usage;
	c._type = _type;
	c._data = _data;
}
