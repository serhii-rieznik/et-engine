/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/vertexarrayobject.h>

namespace et
{

	class RenderState;
	class VertexDeclaration;

	class VertexBufferFactory
	{
	public:
		VertexBufferFactory(RenderState& rs) :
			_rs(rs) { }

		VertexBuffer createVertexBuffer(const std::string& name,
			VertexArray::Pointer data, BufferDrawType drawType);
		
		IndexBuffer createIndexBuffer(const std::string& name,
			IndexArray::Pointer data, BufferDrawType drawType);
		
		VertexArrayObject createVertexArrayObject(const std::string& name);

		VertexArrayObject createVertexArrayObject(const std::string& name, 
			VertexArray::Pointer vertexData, BufferDrawType vertexDrawType,
			IndexArray::Pointer indexData, BufferDrawType indexDrawType);

	private:
		VertexBufferFactory(const VertexBufferFactory& r) : _rs(r._rs)
			{ }

		VertexBufferFactory& operator = (const VertexBufferFactory&)
			{ return *this; }

		RenderState& _rs;
	};

}