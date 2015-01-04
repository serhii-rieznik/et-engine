/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/apiobject.h>
#include <et/vertexbuffer/indexarray.h>

namespace et
{
	class RenderState;
	class IndexBufferData : public APIObject
	{
	public:
		IndexBufferData(RenderContext* rc, IndexArray::Pointer i, BufferDrawType drawType,
			const std::string& name = emptyString);
		
		~IndexBufferData();

		PrimitiveType primitiveType() const
			{ return _primitiveType; }
		
		DataType dataType() const
			{ return _dataType; }

		void* indexOffset(size_t offset) const;

		size_t size() const
			{ return _size; }

		void setSourceTag(size_t tag)
			{_sourceTag = tag; }

		size_t sourceTag() const
			{ return _sourceTag; }

		void setData(const IndexArray::Pointer& i);
		
		void overridePrimitiveType(PrimitiveType);

	private:
		void setProperties(const IndexArray::Pointer& i);
		void build(const IndexArray::Pointer& i);
		
		void internal_setData(const unsigned char* data, size_t size);

	private:
		RenderContext* _rc = nullptr;
		size_t _size = 0;
		size_t _sourceTag = 0;
		DataType _dataType = DataType::UnsignedChar;
		PrimitiveType _primitiveType = PrimitiveType::Points;
		IndexArrayFormat _format = IndexArrayFormat::Format_16bit;
		BufferDrawType _drawType = BufferDrawType::Static;
	};
	
	typedef IntrusivePtr<IndexBufferData> IndexBuffer;
}
