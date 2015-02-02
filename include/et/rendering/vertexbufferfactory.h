/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/apiobjectfactory.h>
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
		
		IndexBuffer createIndexBuffer(const std::string&, IndexArray::Pointer, BufferDrawType);
		
		VertexArrayObject createVertexArrayObject(const std::string&);

		VertexArrayObject createVertexArrayObject(const std::string&, VertexArray::Pointer, BufferDrawType,
			IndexArray::Pointer, BufferDrawType);
		VertexArrayObject createVertexArrayObject(const std::string&, VertexStorage::Pointer, BufferDrawType,
			IndexArray::Pointer, BufferDrawType);

	private:
		ET_DENY_COPY(VertexBufferFactory)
	};

}
