/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/indexarray.h>

namespace et
{
	class RenderState;
	class IndexBuffer : public Object
	{
	public:
		ET_DECLARE_POINTER(IndexBuffer);

	public:
		IndexBuffer(IndexArray::Pointer i, BufferDrawType drawType, const std::string& name);
		~IndexBuffer();

		PrimitiveType primitiveType() const
			{ return _primitiveType; }
		
		IndexArrayFormat format() const
			{ return _format; }

		DataFormat dataFormat() const
			{ return _dataType; }

		void* indexOffset(uint32_t offset) const;

		uint32_t size() const
			{ return _size; }

		void setSourceObjectName(const std::string& tag)
			{ _sourceObjectName = tag; }

		const std::string& sourceObjectName() const
			{ return _sourceObjectName; }

		void bind();

		void setData(const IndexArray::Pointer& i);

		void clear();
		
		void overridePrimitiveType(PrimitiveType);

	private:
		void setProperties(const IndexArray::Pointer& i);
		void build(const IndexArray::Pointer& i);
		
		void internal_setData(const unsigned char* data, uint32_t size);

		uint32_t _ah = 0;
		uint32_t apiHandle() const { return _ah; }
		void setAPIHandle(uint32_t ah) { _ah = ah; }

	private:
		uint32_t _size = 0;
		std::string _sourceObjectName = 0;
		DataFormat _dataType = DataFormat::UnsignedChar;
		PrimitiveType _primitiveType = PrimitiveType::Points;
		IndexArrayFormat _format = IndexArrayFormat::Format_16bit;
		BufferDrawType _drawType = BufferDrawType::Static;
	};
}
