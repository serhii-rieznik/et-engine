/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/vertexbufferfactory.h>

using namespace et;

VertexBufferFactory::VertexBufferFactory(et::RenderContext* rc) :
	APIObjectFactory(rc)
{
}

VertexBuffer VertexBufferFactory::createVertexBuffer(const std::string& name, VertexArray::Pointer data,
	BufferDrawType vertexDrawType)
{
	VertexBuffer vb = VertexBuffer::create(renderContext(), data->generateDescription(), vertexDrawType, name);
	vb->setSourceTag(reinterpret_cast<size_t>(data.ptr()));
	return vb;
}

IndexBuffer VertexBufferFactory::createIndexBuffer(const std::string& name, IndexArray::Pointer data,
	BufferDrawType drawType)
{
	IndexBuffer ib = IndexBuffer::create(renderContext(), data, drawType, name);
	ib->setSourceTag(reinterpret_cast<size_t>(data.ptr()));
	return ib;
}

VertexArrayObject VertexBufferFactory::createVertexArrayObject(const std::string& name)
{
	return VertexArrayObject::create(renderContext(), name);
}

VertexArrayObject VertexBufferFactory::createVertexArrayObject(const std::string& name, 
	VertexArray::Pointer vertexData, BufferDrawType vertexDrawType, IndexArray::Pointer indexData,
	BufferDrawType indexDrawType)
{
	VertexArrayObject result = VertexArrayObject::create(renderContext(), name);

	result->setBuffers(createVertexBuffer(name + "-vb", vertexData, vertexDrawType),
		createIndexBuffer(name + "-ib", indexData, indexDrawType));

	return result;
}
