/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/opengl/openglcaps.h>
#include <et/rendering/rendercontext.h>
#include <et/apiobjects/vertexbufferdata.h>

using namespace et;

VertexBufferData::VertexBufferData(RenderContext* rc, const VertexArray::Description& desc,
	BufferDrawType vertexDrawType, const std::string& aName) : Object(aName), _rc(rc), _vertexBuffer(0),
	_decl(desc.declaration), _dataSize(0), _sourceTag(0), _drawType(vertexDrawType)
{
	glGenBuffers(1, &_vertexBuffer);
	setData(desc.data.data(), desc.data.dataSize());
}

VertexBufferData::VertexBufferData(RenderContext* rc, const VertexDeclaration& decl, const void* vertexData,
	size_t vertexDataSize, BufferDrawType vertexDrawType, const std::string& aName) : Object(aName), _rc(rc),
	_vertexBuffer(0), _decl(decl), _dataSize(0), _sourceTag(0), _drawType(vertexDrawType)
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
	_rc->renderState().bindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
	
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(_dataSize), nullptr, drawTypeValue(_drawType));
	checkOpenGLError("glBufferData(GL_ARRAY_BUFFER, %u, 0x%08X, %d)", _dataSize, data, _drawType);
	
	_dataSize = dataSize;
	
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(_dataSize), data, drawTypeValue(_drawType));
	checkOpenGLError("glBufferData(GL_ARRAY_BUFFER, %u, 0x%08X, %d)", _dataSize, data, _drawType);
}

void VertexBufferData::map(void** data, size_t offset, size_t dataSize, MapBufferMode mode)
{
	ET_ASSERT(data != nullptr);
	
	static const GLenum accessFlags3x[MapBufferMode_max] =
		{ GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT };
	
	_rc->renderState().bindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
	
#if defined(GL_ARB_map_buffer_range) || defined(GL_EXT_map_buffer_range)
	
	GLbitfield access = GL_MAP_UNSYNCHRONIZED_BIT | accessFlags3x[mode];
	
	if (mode == MapBufferMode_WriteOnly)
		access |= GL_MAP_INVALIDATE_BUFFER_BIT;
	
	*data = glMapBufferRange(GL_ARRAY_BUFFER, offset, dataSize, access);
	checkOpenGLError("glMapBufferRange(GL_ARRAY_BUFFER, %lu, %lu, %d)", offset, dataSize, mode);

#elif defined(GL_READ_ONLY) && defined(GL_WRITE_ONLY) && defined(GL_READ_WRITE)
	
	static const GLenum accessFlags2x[MapBufferMode_max] =
		{ GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE };
	
	*data = reinterpret_cast<uint8_t*>(glMapBuffer(GL_ARRAY_BUFFER, accessFlags2x[mode])) + offset;
	checkOpenGLError("glMapBuffer(GL_ARRAY_BUFFER, %d)", mode);
	
#else
	log::error("Invalid call to glMapBuffer.");
#endif
}

void VertexBufferData::unmap()
{
	glUnmapBuffer(GL_ARRAY_BUFFER);
	checkOpenGLError("glUnmapBuffer(GL_ARRAY_BUFFER)");
}

void VertexBufferData::serialize(std::ostream&)
{
	ET_FAIL("Unsupported");
}

void VertexBufferData::deserialize(std::istream&)
{
	ET_FAIL("Unsupported");
}
