/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
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
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create VertexBuffer in console application.")
#else
	glGenBuffers(1, &_vertexBuffer);
	setData(desc.data.data(), desc.data.dataSize());
#endif
}

VertexBufferData::VertexBufferData(RenderContext* rc, const VertexDeclaration& decl, const void* vertexData,
	size_t vertexDataSize, BufferDrawType vertexDrawType, const std::string& aName) : Object(aName), _rc(rc),
	_vertexBuffer(0), _decl(decl), _dataSize(0), _sourceTag(0), _drawType(vertexDrawType)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create VertexBuffer in console application.")
#else
	glGenBuffers(1, &_vertexBuffer);
	setData(vertexData, vertexDataSize);
#endif
}

VertexBufferData::~VertexBufferData()
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (_vertexBuffer)
		glDeleteBuffers(1, &_vertexBuffer);
	_rc->renderState().vertexBufferDeleted(_vertexBuffer);
#endif
}

void VertexBufferData::setData(const void* data, size_t dataSize)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_rc->renderState().bindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
	
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(_dataSize), nullptr, drawTypeValue(_drawType));
	checkOpenGLError("glBufferData(GL_ARRAY_BUFFER, %u, 0x%08X, %d)", _dataSize, data, _drawType);
	
	_dataSize = dataSize;
	
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(_dataSize), data, drawTypeValue(_drawType));
	checkOpenGLError("glBufferData(GL_ARRAY_BUFFER, %u, 0x%08X, %d)", _dataSize, data, _drawType);
#endif
}

void* VertexBufferData::map(size_t offset, size_t dataSize, MapBufferMode mode)
{
	ET_ASSERT(!_mapped)
	ET_ASSERT(dataSize > 0)

	void* result = nullptr;
	
#if !defined(ET_CONSOLE_APPLICATION)
		
	_rc->renderState().bindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
	
	static const GLenum accessFlags2x[MapBufferMode_max] =
		{ GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE };

	bool shouldUseMapBuffer = true;

#	if defined(GL_ARB_map_buffer_range) || defined(GL_EXT_map_buffer_range)

		static const GLenum accessFlags3x[MapBufferMode_max] =
			{ GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT };
	
		result = glMapBufferRange(GL_ARRAY_BUFFER, offset, dataSize, accessFlags3x[mode]);
		checkOpenGLError("glMapBufferRange(GL_ARRAY_BUFFER, %lu, %lu, %d)", offset, dataSize, mode);

		shouldUseMapBuffer = (result == nullptr);

#	endif

	if (shouldUseMapBuffer)
	{
		if (accessFlags2x == 0)
		{
			log::error("glMapBuffer access flag %d is not supported.", mode);
			return nullptr;
		}
		
		result = reinterpret_cast<uint8_t*>(glMapBuffer(GL_ARRAY_BUFFER, accessFlags2x[mode])) + offset;
		checkOpenGLError("glMapBuffer(GL_ARRAY_BUFFER, %d)", mode);
	}

	_mapped = true;

#endif

	return result;
}

void VertexBufferData::unmap()
{
#if !defined(ET_CONSOLE_APPLICATION)
	glUnmapBuffer(GL_ARRAY_BUFFER);
	checkOpenGLError("glUnmapBuffer(GL_ARRAY_BUFFER)");

	_mapped = false;
#endif
}

void VertexBufferData::serialize(std::ostream&)
{
	ET_FAIL("Unsupported");
}

void VertexBufferData::deserialize(std::istream&)
{
	ET_FAIL("Unsupported");
}
