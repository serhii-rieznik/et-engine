/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/opengl/opengl.h>
#include <et/rendering/rendercontext.h>

using namespace et;

IndexBuffer::IndexBuffer(RenderContext* rc, IndexArray::Pointer i, BufferDrawType drawType,
	const std::string& aName) : APIObject(aName), _size(i->actualSize()), _sourceObjectName(i->name()),
	_drawType(drawType)
{
	build(i);
}

IndexBuffer::~IndexBuffer()
{
	uint32_t buffer = apiHandle();
	if ((buffer != 0) && glIsBuffer(buffer))
	{
#	if (ET_EXPOSE_OLD_RENDER_STATE)
		_rc->renderState()->indexBufferDeleted(buffer);
#	endif
		glDeleteBuffers(1, &buffer);
	}
}

void IndexBuffer::bind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, apiHandle());
}

void IndexBuffer::setProperties(const IndexArray::Pointer& i)
{
	_size = i->actualSize();
	_primitiveType = i->primitiveType();
	_format = i->format();

	switch (_format)
	{
		case IndexArrayFormat::Format_8bit:
		{
			_dataType = DataFormat::UnsignedChar;
			break;
		}
			
		case IndexArrayFormat::Format_16bit:
		{
			_dataType = DataFormat::UnsignedShort;
			break;
		}
			
		case IndexArrayFormat::Format_32bit:
		{
			_dataType = DataFormat::UnsignedInt;
			break;
		}
			
		default:
			ET_FAIL_FMT("Invalid IndexArrayFormat value: %u", static_cast<uint32_t>(_format));
	}
}

void IndexBuffer::build(const IndexArray::Pointer& i)
{
	ET_ASSERT(i.valid());

	if (apiHandleInvalid())
	{
		uint32_t buffer = 0;
		
		glGenBuffers(1, &buffer);
		checkOpenGLError("glGenBuffers(1, &_indexBuffer)");
		
		setAPIHandle(buffer);
	}

	setProperties(i);
	internal_setData(i->data(), static_cast<uint32_t>(i->format()) * _size);
}

void IndexBuffer::internal_setData(const unsigned char* data, uint32_t size)
{
	ET_ASSERT(size > 0);

	bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, drawTypeValue(_drawType));
	checkOpenGLError("glBufferData(GL_ELEMENT_ARRAY_BUFFER, %u, 0x%08X, ..,)", size, data);
}

void* IndexBuffer::indexOffset(uint32_t offset) const
{
	return reinterpret_cast<void*>(static_cast<uintptr_t>(_format) * offset);
}

void IndexBuffer::setData(const IndexArray::Pointer& i)
{
	build(i);
}

void IndexBuffer::clear()
{
	_size = 0;
	internal_setData(nullptr, 0);
}

void IndexBuffer::overridePrimitiveType(PrimitiveType pt)
{
	_primitiveType = pt;
}
