/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/vertexbuffer/vertexdeclaration.h>

namespace et
{
	class VertexDataChunkData : public Shared
	{
	public:
		VertexDataChunkData(std::istream& stream);
		VertexDataChunkData(VertexAttributeUsage usage, DataType type, size_t size);

		void resize(size_t);
		void fitToSize(size_t);

		char* data()
			{ return _data.binary(); }

		const char* data() const
			{ return _data.binary(); }

		const size_t size() const
			{ return _data.size(); }

		const size_t dataSize() const
			{ return _data.dataSize(); }

		const size_t typeSize() const
			{ return dataTypeSize(_type); }

		VertexAttributeUsage usage() const
			{ return _usage; }

		DataType type() const
			{ return _type; }

		void copyTo(VertexDataChunkData&) const;

	private:
		VertexAttributeUsage _usage = VertexAttributeUsage::Position;
		DataType _type = DataType::Float;
		BinaryDataStorage _data;
	};

	class VertexDataChunk : public IntrusivePtr<VertexDataChunkData>
	{
	public:
		VertexDataChunk()
			{ }

		VertexDataChunk(std::istream& stream) : 
			IntrusivePtr<VertexDataChunkData>(etCreateObject<VertexDataChunkData>(stream)) { }

		VertexDataChunk(VertexAttributeUsage usage, DataType type, size_t size) : 
			IntrusivePtr<VertexDataChunkData>(etCreateObject<VertexDataChunkData>(usage, type, size)) { }

		template <typename T>
		RawDataAcessor<T> accessData(size_t elementOffset) 
		{
			return valid() ? RawDataAcessor<T>(ptr()->data(), ptr()->dataSize(), ptr()->typeSize(),
				elementOffset * ptr()->typeSize()) : RawDataAcessor<T>();
		}
		
		template <typename T>
		const RawDataAcessor<T> accessData(size_t elementOffset) const
		{
			return valid() ? RawDataAcessor<T>(ptr()->data(), ptr()->dataSize(), ptr()->typeSize(),
				elementOffset * ptr()->typeSize()) : RawDataAcessor<T>();
		}
	};

	typedef std::vector<VertexDataChunk> VertexDataChunkList;
}
