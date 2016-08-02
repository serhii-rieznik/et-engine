/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/opengl/opengl_indexbuffer.h>
#include <et/rendering/opengl/opengl.h>

using namespace et;

OpenGLIndexBuffer::OpenGLIndexBuffer(IndexArray::Pointer i, BufferDrawType dt, const std::string& aName) :
    IndexBuffer(i, dt, aName)
{
	build(i);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer()
{
	uint32_t buffer = apiHandle();
	if ((buffer != 0) && glIsBuffer(buffer))
	{
#	if (ET_EXPOSE_OLD_RENDER_STATE)
		_rc->renderState()->OpenGLIndexBufferDeleted(buffer);
#	endif
		glDeleteBuffers(1, &buffer);
	}
}

void OpenGLIndexBuffer::bind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, apiHandle());
}

void OpenGLIndexBuffer::build(const IndexArray::Pointer& i)
{
	ET_ASSERT(i.valid());

	if (apiHandle() == 0)
	{
		uint32_t buffer = 0;
		
		glGenBuffers(1, &buffer);
		checkOpenGLError("glGenBuffers(1, &_OpenGLIndexBuffer)");
		
		setAPIHandle(buffer);
	}

	internal_setData(i->data(), static_cast<uint32_t>(i->format()) * size());
}

void OpenGLIndexBuffer::internal_setData(const unsigned char* data, uint32_t size)
{
	ET_ASSERT(size > 0);

	bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, drawTypeValue(drawType()));
	checkOpenGLError("glBufferData(GL_ELEMENT_ARRAY_BUFFER, %u, 0x%08X, ..,)", size, data);
}

void* OpenGLIndexBuffer::indexOffset(uint32_t offset) const
{
	return reinterpret_cast<void*>(static_cast<uintptr_t>(format()) * offset);
}

void OpenGLIndexBuffer::setData(const IndexArray::Pointer& i)
{
	build(i);
}

void OpenGLIndexBuffer::clear()
{
	setSize(0);
	internal_setData(nullptr, 0);
}
