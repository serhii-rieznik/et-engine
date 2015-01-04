/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/apiobjects/apiobjectfactory.h>
#include <et/apiobjects/vertexarrayobject.h>

namespace et
{
	class VertexDeclaration;

	class VertexBufferFactory : public APIObjectFactory
	{
	public:
		ET_DECLARE_POINTER(VertexBufferFactory)
		
	public:
		VertexBufferFactory(RenderContext* rc);

		VertexBuffer createVertexBuffer(const std::string& name,
			VertexArray::Pointer data, BufferDrawType drawType);
		
		IndexBuffer createIndexBuffer(const std::string& name,
			IndexArray::Pointer data, BufferDrawType drawType);
		
		VertexArrayObject createVertexArrayObject(const std::string& name);

		VertexArrayObject createVertexArrayObject(const std::string& name, 
			VertexArray::Pointer vertexData, BufferDrawType vertexDrawType,
			IndexArray::Pointer indexData, BufferDrawType indexDrawType);

	private:
		ET_DENY_COPY(VertexBufferFactory)
	};

}
