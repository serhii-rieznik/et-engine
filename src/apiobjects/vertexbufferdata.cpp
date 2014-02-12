/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et/apiobjects/vertexbufferdata.h>

using namespace et;

VertexBufferData::VertexBufferData(RenderContext* rc, const VertexArray::Description& desc,
	BufferDrawType vertexDrawType, const std::string& aName) : Object(aName), _rc(rc), _vertexBuffer(0),
	_decl(desc.declaration), _dataSize(desc.data.dataSize()), _sourceTag(0), _drawType(vertexDrawType)
{
	glGenBuffers(1, &_vertexBuffer);
	setData(desc.data.data(), desc.data.dataSize());
}

VertexBufferData::VertexBufferData(RenderContext* rc, const VertexDeclaration& decl, const void* vertexData,
	size_t vertexDataSize, BufferDrawType vertexDrawType, const std::string& aName) : Object(aName), _rc(rc),
	_vertexBuffer(0), _decl(decl), _dataSize(vertexDataSize), _sourceTag(0), _drawType(vertexDrawType)
{
	glGenBuffers(1, &_vertexBuffer);
	setData(vertexData, vertexDataSize);
}

VertexBufferData::~VertexBufferData()
{
	if (_vertexBuffer)
		glDeleteBuffers(1, &_vertexBuffer);

	_rc->renderState().vertexBufferDeleted(_vertexBuffer);
}

void VertexBufferData::setData(const void* data, size_t dataSize)
{
	_dataSize = dataSize;
	
	if (_dataSize > 0)
	{
		_rc->renderState().bindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(_dataSize), data, drawTypeValue(_drawType));
		checkOpenGLError("glBufferData(GL_ARRAY_BUFFER, %u, 0x%08X, ..,)", _dataSize, data);
	}
}

void VertexBufferData::serialize(std::ostream&)
{
	ET_FAIL("Unsupported");
}

void VertexBufferData::deserialize(std::istream&)
{
	ET_ASSERT("Unsupported");
}