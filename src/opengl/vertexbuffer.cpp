/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/opengl/opengl.h>
#include <et/rendering/rendercontext.h>

using namespace et;

VertexBuffer::VertexBuffer(RenderContext* rc, const VertexDeclaration& decl,
	const BinaryDataStorage& data, BufferDrawType vertexDrawType, const std::string& aName) : 
	APIObject(aName), _rc(rc), _decl(decl),  _drawType(vertexDrawType)
{
	GLuint buffer = 0;
	glGenBuffers(1, &buffer);
	setAPIHandle(buffer);
	setData(data.data(), data.dataSize());
}

VertexBuffer::VertexBuffer(RenderContext* rc, const VertexArray::Description& desc, BufferDrawType drawType,
	const std::string& aName) : VertexBuffer(rc, desc.declaration, desc.data, drawType, aName) { }

VertexBuffer::~VertexBuffer()
{
	uint32_t buffer = apiHandle();
	if (buffer != 0)
	{
		_rc->renderState().vertexBufferDeleted(buffer);
		glDeleteBuffers(1, &buffer);
	}
}

void VertexBuffer::setData(const void* data, size_t dataSize, bool invalidateExistingData)
{
	_rc->renderState().bindBuffer(GL_ARRAY_BUFFER, apiHandle());
	
	if (invalidateExistingData)
	{
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, drawTypeValue(_drawType));
		checkOpenGLError("glBufferData(GL_ARRAY_BUFFER, %u, 0x%08X, %d)", _dataSize, data, _drawType);
	}
	
	_dataSize = dataSize;
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(_dataSize), data, drawTypeValue(_drawType));
	checkOpenGLError("glBufferData(GL_ARRAY_BUFFER, %u, 0x%08X, %d)", _dataSize, data, _drawType);
}

void VertexBuffer::setDataWithOffset(const void* data, size_t offset, size_t dataSize)
{
	_rc->renderState().bindBuffer(GL_ARRAY_BUFFER, apiHandle());
	
	glBufferSubData(GL_ARRAY_BUFFER, offset, dataSize, data);
	checkOpenGLError("glBufferSubData(GL_ARRAY_BUFFER, %llu, %llu, 0x%08X)", uint64_t(offset),
		uint64_t(_dataSize), data);
}

void* VertexBuffer::map(size_t offset, size_t dataSize, uint32_t options)
{
	ET_ASSERT(!_mapped);
	ET_ASSERT(dataSize > 0);

	void* result = nullptr;
	
	_rc->renderState().bindBuffer(GL_ARRAY_BUFFER, apiHandle());
	
	bool shouldUseMapBuffer = true;

#if defined(GL_ARB_map_buffer_range) || defined(GL_EXT_map_buffer_range)

	uint32_t mbrOptions = 0;
	
	if (options & MapBufferOptions::Read)
		mbrOptions |= GL_MAP_READ_BIT;
	if (options & MapBufferOptions::Write)
		mbrOptions |= GL_MAP_WRITE_BIT;
	if (options & MapBufferOptions::Unsynchronized)
		mbrOptions |= GL_MAP_UNSYNCHRONIZED_BIT;
	if (options & MapBufferOptions::InvalidateBuffer)
		mbrOptions |= GL_MAP_INVALIDATE_BUFFER_BIT;
	if (options & MapBufferOptions::InvalidateRange)
		mbrOptions |= GL_MAP_INVALIDATE_RANGE_BIT;
	
#	if (ET_DEBUG)
		GLint bufferSize = 0;
		glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
		ET_ASSERT(offset + dataSize <= static_cast<size_t>(bufferSize));
#	endif

	result = glMapBufferRange(GL_ARRAY_BUFFER, offset, dataSize, mbrOptions);
	checkOpenGLError("glMapBufferRange(GL_ARRAY_BUFFER, %lu, %lu, %X)", offset, dataSize, mbrOptions);

	shouldUseMapBuffer = (result == nullptr);

#endif

	if (shouldUseMapBuffer)
	{
		uint32_t mbOptions = 0;
		if ((options & MapBufferOptions::Read) && (options & MapBufferOptions::Write))
			mbOptions = GL_READ_WRITE;
		else if (options & MapBufferOptions::Read)
			mbOptions = GL_READ_ONLY;
		else if (options & MapBufferOptions::Write)
			mbOptions = GL_WRITE_ONLY;
		result = reinterpret_cast<uint8_t*>(glMapBuffer(GL_ARRAY_BUFFER, mbOptions)) + offset;
		checkOpenGLError("glMapBuffer(GL_ARRAY_BUFFER, %X)", mbOptions);
	}

	_mapped = true;

	return result;
}

void VertexBuffer::unmap()
{
	ET_ASSERT(_mapped);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	checkOpenGLError("glUnmapBuffer(GL_ARRAY_BUFFER)");
	_mapped = false;
}

void VertexBuffer::clear()
{
	_dataSize = 0;
	setData(nullptr, 0);
}
