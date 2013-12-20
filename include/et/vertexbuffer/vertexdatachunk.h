/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
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
		VertexDataChunkData(VertexAttributeUsage usage, VertexAttributeType type, size_t size);

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
			{ return vertexAttributeTypeSize(_type); }

		VertexAttributeUsage usage() const
			{ return _usage; }

		VertexAttributeType type() const
			{ return _type; }

		void serialize(std::ostream& stream);
		
		void copyTo(VertexDataChunkData&);

	private:
		VertexAttributeUsage _usage;
		VertexAttributeType _type;
		BinaryDataStorage _data;
	};

	class VertexDataChunk : public IntrusivePtr<VertexDataChunkData>
	{
	public:
		VertexDataChunk()
			{ }

		VertexDataChunk(std::istream& stream) : 
			IntrusivePtr<VertexDataChunkData>(new VertexDataChunkData(stream))
			{ }

		VertexDataChunk(VertexAttributeUsage usage, VertexAttributeType type, size_t size) : 
			IntrusivePtr<VertexDataChunkData>(new VertexDataChunkData(usage, type, size))
			{ }

		template <typename T>
		RawDataAcessor<T> accessData(size_t elementOffset) 
		{
			return valid() ? RawDataAcessor<T>(ptr()->data(), ptr()->dataSize(), ptr()->typeSize(),
				elementOffset * ptr()->typeSize()) : RawDataAcessor<T>();
		}
	};

	typedef std::vector<VertexDataChunk> VertexDataChunkList;
}