/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

/*
 
#include <et/rendering/vertexbufferfactory.h>

using namespace et;

VertexBuffer::Pointer VertexBufferFactory::createVertexBuffer(const std::string& name,
	VertexArray::Pointer data, BufferDrawType drawType)
{
	VertexBuffer::Pointer vb = VertexBuffer::Pointer::create(data->generateDescription(),
		drawType, name);
	vb->setSourceObjectName(data->name());
	return vb;
}

VertexBuffer::Pointer VertexBufferFactory::createVertexBuffer(const std::string& name,
	VertexStorage::Pointer data, BufferDrawType drawType)
{
	VertexBuffer::Pointer vb = VertexBuffer::Pointer::create(data->declaration(), data->data(), drawType, name);
	vb->setSourceObjectName(data->name());
	return vb;
}

IndexBuffer::Pointer VertexBufferFactory::createIndexBuffer(const std::string& name, IndexArray::Pointer data,
	BufferDrawType drawType)
{
	IndexBuffer::Pointer ib = IndexBuffer::Pointer::create(data, drawType, name);
	ib->setSourceObjectName(data->name());
	return ib;
}

VertexArrayObject::Pointer VertexBufferFactory::createVertexArrayObject(const std::string& name)
{
	return VertexArrayObject::Pointer::create(name);
}

VertexArrayObject::Pointer VertexBufferFactory::createVertexArrayObject(const std::string& name, 
	VertexArray::Pointer vertexData, BufferDrawType vertexDrawType, IndexArray::Pointer indexData,
	BufferDrawType indexDrawType)
{
	VertexArrayObject::Pointer result = VertexArrayObject::Pointer::create(name);

	result->setBuffers(createVertexBuffer(name + "-vb", vertexData, vertexDrawType),
		createIndexBuffer(name + "-ib", indexData, indexDrawType));

	return result;
}

VertexArrayObject::Pointer VertexBufferFactory::createVertexArrayObject(const std::string& name,
	VertexStorage::Pointer vertexData, BufferDrawType vertexDrawType, IndexArray::Pointer indexData,
	BufferDrawType indexDrawType)
{
	VertexArrayObject::Pointer result = VertexArrayObject::Pointer::create(name);
	
	auto vb = createVertexBuffer(name + "-vb", vertexData, vertexDrawType);
	auto ib = IndexBuffer::Pointer();
	
	if (indexData.valid())
		ib = createIndexBuffer(name + "-ib", indexData, indexDrawType);

	result->setBuffers(vb, ib);
	
	return result;
}

VertexArrayObject::Pointer VertexBufferFactory::createVertexArrayObject(const std::string& name, VertexStorage::Pointer vertexData,
	BufferDrawType vertexDrawType, IndexBuffer::Pointer ib)
{
	VertexArrayObject::Pointer result = VertexArrayObject::Pointer::create(name);
	result->setBuffers(createVertexBuffer(name + "-vb", vertexData, vertexDrawType), ib);
	return result;
}

*/
