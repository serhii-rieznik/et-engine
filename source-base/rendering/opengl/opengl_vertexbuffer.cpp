/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/opengl/opengl_vertexbuffer.h>
#include <et/rendering/opengl/opengl.h>
#include <et/core/containers.h>

namespace et
{

class OpenGLVertexBufferPrivate
{
public:
	GLuint buffer = 0;
	size_t dataSize = 0;
	bool mapped = false;
};

OpenGLVertexBuffer::OpenGLVertexBuffer(const VertexDeclaration& decl, const BinaryDataStorage& data,
	BufferDrawType drawType, const std::string& aName) : VertexBuffer(decl, drawType, aName)
{
	ET_PIMPL_INIT(OpenGLVertexBuffer);

	_private->dataSize = data.size();
	glGenBuffers(1, &_private->buffer);
	setData(data.data(), data.dataSize(), false);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
	if ((_private->buffer != 0) && glIsBuffer(_private->buffer))
	{
#	if (ET_EXPOSE_OLD_RENDER_STATE)
		_rc->renderState()->vertexBufferDeleted(_private->buffer);
#	endif
		glDeleteBuffers(1, &_private->buffer);
	}

	ET_PIMPL_FINALIZE(OpenGLVertexBuffer);
}

void OpenGLVertexBuffer::bind()
{
	glBindBuffer(GL_ARRAY_BUFFER, _private->buffer);

	for (uint32_t i = 0; i < VertexAttributeUsage_max; ++i)
	{
		if (declaration().has(static_cast<VertexAttributeUsage>(i)))
		{
			glEnableVertexAttribArray(i);
		}
		else
		{
			glDisableVertexAttribArray(i);
		}
		checkOpenGLError("glEnableVertexAttribArray");
	}

	for (size_t i = 0; i < declaration().numElements(); ++i)
	{
		const VertexElement& e = declaration().element(i);
		uint32_t dataOffset = i * (declaration().interleaved() ? declaration().dataSize() : dataTypeSize(e.type()));

		auto usage = GLuint(e.usage());
		auto components = static_cast<GLint>(e.components());
		auto dataFormat = dataFormatValue(e.dataFormat());
		auto stride = e.stride();
		auto ptr = reinterpret_cast<GLvoid*>(uintptr_t(e.offset()));

		if (e.dataFormat() == DataFormat::Int)
		{
			glVertexAttribIPointer(usage, components, dataFormat, stride, ptr);
		}
		else if (e.dataFormat() == DataFormat::Float)
		{
			glVertexAttribPointer(usage, components, dataFormat, false, stride, ptr);
		}
	}
}

void OpenGLVertexBuffer::setData(const void* data, size_t dataSize, bool invalidateExistingData)
{
	bind();
	
	if (invalidateExistingData)
	{
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, drawTypeValue(drawType()));
		checkOpenGLError("glBufferData(GL_ARRAY_BUFFER, %u, 0x%08X, %d)", _private->dataSize, data, drawType());
	}
	
	_private->dataSize = dataSize;
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(_private->dataSize), data, drawTypeValue(drawType()));
	checkOpenGLError("glBufferData(GL_ARRAY_BUFFER, %u, 0x%08X, %d)", _private->dataSize, data, drawType());
}

void OpenGLVertexBuffer::setDataWithOffset(const void* data, size_t offset, size_t dataSize)
{
	bind();

	glBufferSubData(GL_ARRAY_BUFFER, offset, dataSize, data);
	checkOpenGLError("glBufferSubData(GL_ARRAY_BUFFER, %llu, %llu, 0x%08X)", uint64_t(offset),
		uint64_t(_private->dataSize), data);
}

uint64_t OpenGLVertexBuffer::dataSize()
{
	return _private->dataSize;
}

void* OpenGLVertexBuffer::map(size_t offset, size_t dataSize, uint32_t options)
{
	ET_ASSERT(!_private->mapped);
	ET_ASSERT(dataSize > 0);

	void* result = nullptr;
	
	bind();

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

	_private->mapped = true;

	return result;
}

bool OpenGLVertexBuffer::mapped() const
{
	return _private->mapped;
}

void OpenGLVertexBuffer::unmap()
{
	ET_ASSERT(_private->mapped);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	checkOpenGLError("glUnmapBuffer(GL_ARRAY_BUFFER)");
	_private->mapped = false;
}

void OpenGLVertexBuffer::clear()
{
	_private->dataSize = 0;
	setData(nullptr, 0, false);
}

}
