/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/vertexbuffer/indexarray.h>

namespace et
{
	class RenderState;
	class IndexBufferData : public Object
	{
	public:
		IndexBufferData(RenderState& rs, IndexArray::Pointer i, BufferDrawType drawType,
			const std::string& name = std::string());
		
		~IndexBufferData();

		uint32_t primitiveType() const
			{ return _primitiveType; }
		
		uint32_t dataType() const
			{ return _dataType; }

		void* indexOffset(size_t offset) const;

		uint32_t glID() const
			{ return _indexBuffer; }

		size_t size() const
			{ return _size; }

		void setSourceTag(size_t tag)
			{_sourceTag = tag; }

		size_t sourceTag() const
			{ return _sourceTag; }

		void setData(const IndexArray::Pointer& i);

	private:
		void setProperties(const IndexArray::Pointer& i);
		void build(const IndexArray::Pointer& i);
		
		void internal_setData(const unsigned char* data, size_t size);

	private:
		RenderState& _rs;
		size_t _size;
		size_t _sourceTag;
		uint32_t _indexBuffer;
		uint32_t _dataType;
		uint32_t _primitiveType;
		IndexArrayFormat _format;
		BufferDrawType _drawType;
	};
}
