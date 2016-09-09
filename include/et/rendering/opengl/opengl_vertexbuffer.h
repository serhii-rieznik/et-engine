/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/vertexbuffer.h>

namespace et
{
	class OpenGLVertexBufferPrivate;
	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		ET_DECLARE_POINTER(OpenGLVertexBuffer);
		
	public:
		OpenGLVertexBuffer(const VertexDeclaration&, const BinaryDataStorage&, BufferDrawType, const std::string&);
		~OpenGLVertexBuffer();

		void bind() override;
		
		void setData(const void* data, uint32_t dataSize, bool invalidateExistingData) override;
		void setDataWithOffset(const void* data, uint32_t offset, uint32_t dataSize) override;

		uint64_t dataSize() override;

		void* map(uint32_t offset, uint32_t dataSize, uint32_t options /* see MapBufferOptions */) override;
		bool mapped() const override;
		void unmap() override;

		void clear() override;

	private:
		ET_DECLARE_PIMPL(OpenGLVertexBuffer, 32)
	};
}
