/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/apiobjects/vertexbufferfactory.h>

using namespace et;

VertexBuffer VertexBufferFactory::createVertexBuffer(const std::string& name, VertexArray::Pointer data,
	BufferDrawType vertexDrawType)
{
	VertexBuffer vb(new VertexBufferData(_rs, data->generateDescription(), vertexDrawType, name));
	vb->setSourceTag(reinterpret_cast<size_t>(data.ptr()));
	return vb;
}

IndexBuffer VertexBufferFactory::createIndexBuffer(const std::string& name, IndexArray::Pointer data,
	BufferDrawType drawType)
{
	IndexBuffer ib(new IndexBufferData(_rs, data, drawType, name));
	ib->setSourceTag(reinterpret_cast<size_t>(data.ptr()));
	return ib;
}

VertexArrayObject VertexBufferFactory::createVertexArrayObject(const std::string& name)
{
	return VertexArrayObject(new VertexArrayObjectData(_rs, name));
}

VertexArrayObject VertexBufferFactory::createVertexArrayObject(const std::string& name, 
	VertexArray::Pointer vertexData, BufferDrawType vertexDrawType, IndexArray::Pointer indexData,
	BufferDrawType indexDrawType)
{
	VertexArrayObject result(new VertexArrayObjectData(_rs, name));

	result->setBuffers(createVertexBuffer(name + "-vb", vertexData, vertexDrawType),
		createIndexBuffer(name + "-ib", indexData, indexDrawType));

	return result;
}
