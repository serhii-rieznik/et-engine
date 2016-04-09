/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/apiobject.h>
#include <et/rendering/vertexarrayobject.h>

namespace et
{
	class VertexDeclaration;

	class VertexBufferFactory : public APIObjectFactory
	{
	public:
		ET_DECLARE_POINTER(VertexBufferFactory)
		
	public:
		VertexBufferFactory(RenderContext* rc);

		VertexBuffer::Pointer createVertexBuffer(const std::string&, VertexArray::Pointer, BufferDrawType);
		VertexBuffer::Pointer createVertexBuffer(const std::string&, VertexStorage::Pointer, BufferDrawType);
		
		IndexBuffer::Pointer createIndexBuffer(const std::string&, IndexArray::Pointer, BufferDrawType);
		
		VertexArrayObject::Pointer createVertexArrayObject(const std::string&);

		VertexArrayObject::Pointer createVertexArrayObject(const std::string&, VertexArray::Pointer, BufferDrawType,
			IndexArray::Pointer, BufferDrawType);

		VertexArrayObject::Pointer createVertexArrayObject(const std::string&, VertexStorage::Pointer, BufferDrawType,
			IndexArray::Pointer, BufferDrawType);

		VertexArrayObject::Pointer createVertexArrayObject(const std::string&, VertexStorage::Pointer, BufferDrawType,
			IndexBuffer::Pointer);

	private:
		ET_DENY_COPY(VertexBufferFactory)
	};

}
