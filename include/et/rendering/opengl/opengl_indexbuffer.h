/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/indexbuffer.h>

namespace et
{
	class RenderState;
	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		ET_DECLARE_POINTER(OpenGLIndexBuffer);

	public:
		OpenGLIndexBuffer(IndexArray::Pointer i, BufferDrawType drawType, const std::string& name);
		~OpenGLIndexBuffer();

        void bind() override;
        void clear() override;
        
		void* indexOffset(uint32_t offset) const;
		void setData(const IndexArray::Pointer& i);
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
