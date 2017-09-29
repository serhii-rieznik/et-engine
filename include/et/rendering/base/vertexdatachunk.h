/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/rendering/base/vertexdeclaration.h>

namespace et
{
class VertexDataChunkData : public Object
{
public:
	VertexDataChunkData(std::istream& stream);
	VertexDataChunkData(VertexAttributeUsage usage, DataType type, uint32_t size);

	void resize(uint32_t);
	void fitToSize(uint32_t);

	char* data()
	{
		return _data.binary();
	}

	const char* data() const
	{
		return _data.binary();
	}

	uint32_t size() const
	{
		return static_cast<uint32_t>(_data.size());
	}

	uint32_t dataSize() const
	{
		return static_cast<uint32_t>(_data.dataSize());
	}

	uint32_t typeSize() const
	{
		return dataTypeSize(_type);
	}

	VertexAttributeUsage usage() const
	{
		return _usage;
	}

	DataType type() const
	{
		return _type;
	}

	void copyTo(VertexDataChunkData&) const;

private:
	VertexAttributeUsage _usage = VertexAttributeUsage::Position;
	DataType _type = DataType::Float;
	BinaryDataStorage _data;
};

using VertexDataChunk = IntrusivePtr<VertexDataChunkData>;
using VertexDataChunkList = Vector<VertexDataChunk>;
template <typename T>

RawDataAcessor<T> accessData(VertexDataChunk ch, size_t elementOffset)
{
	auto _this = ch.pointer();
	return ch.valid() ? RawDataAcessor<T>(_this->data(), _this->dataSize(), _this->typeSize(),
		elementOffset * _this->typeSize()) : RawDataAcessor<T>();
}

}
