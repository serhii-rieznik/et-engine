/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/apiobjects/vertexbufferfactory.h>

using namespace et;

VertexBufferFactory::VertexBufferFactory(et::RenderContext* rc) :
	APIObjectFactory(rc)
{
}

VertexBuffer VertexBufferFactory::createVertexBuffer(const std::string& name, VertexArray::Pointer data,
	BufferDrawType vertexDrawType)
{
	VertexBuffer vb(new VertexBufferData(renderContext(), data->generateDescription(), vertexDrawType, name));
	vb->setSourceTag(reinterpret_cast<size_t>(data.ptr()));
	return vb;
}

IndexBuffer VertexBufferFactory::createIndexBuffer(const std::string& name, IndexArray::Pointer data,
	BufferDrawType drawType)
{
	IndexBuffer ib(new IndexBufferData(renderContext(), data, drawType, name));
	ib->setSourceTag(reinterpret_cast<size_t>(data.ptr()));
	return ib;
}

VertexArrayObject VertexBufferFactory::createVertexArrayObject(const std::string& name)
{
	return VertexArrayObject(new VertexArrayObjectData(renderContext(), name));
}

VertexArrayObject VertexBufferFactory::createVertexArrayObject(const std::string& name, 
	VertexArray::Pointer vertexData, BufferDrawType vertexDrawType, IndexArray::Pointer indexData,
	BufferDrawType indexDrawType)
{
	VertexArrayObject result(new VertexArrayObjectData(renderContext(), name));

	result->setBuffers(createVertexBuffer(name + "-vb", vertexData, vertexDrawType),
		createIndexBuffer(name + "-ib", indexData, indexDrawType));

	return result;
}
