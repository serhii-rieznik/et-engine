/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et/apiobjects/indexbufferdata.h>

using namespace et;

IndexBufferData::IndexBufferData(RenderContext* rc, IndexArray::Pointer i, BufferDrawType drawType,
	const std::string& aName) : Object(aName), _rc(rc), _size(i->actualSize()), _sourceTag(0),
	_indexBuffer(0), _dataType(0), _primitiveType(0), _format(IndexArrayFormat_Undefined), _drawType(drawType)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create IndexBuffer in console application.")
#else
	build(i);
#endif
}

IndexBufferData::~IndexBufferData()
{
#if !defined(ET_CONSOLE_APPLICATION)
	_rc->renderState().indexBufferDeleted(_indexBuffer);
	
	if (_indexBuffer != 0)
		glDeleteBuffers(1, &_indexBuffer);
#endif
}

void IndexBufferData::setProperties(const IndexArray::Pointer& i)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_size = i->actualSize();
	_format = i->format();

	switch (i->format())
	{
		case IndexArrayFormat_8bit:
		{
			_dataType = GL_UNSIGNED_BYTE;
			break;
		}
		case IndexArrayFormat_16bit:
		{
			_dataType = GL_UNSIGNED_SHORT;
			break;
		}
		case IndexArrayFormat_32bit:
		{
			_dataType = GL_UNSIGNED_INT;
			break;
		}
		default:
			ET_FAIL("Invalud IndexArrayFormat value");
	}

	_primitiveType = primitiveTypeValue(i->primitiveType());
#endif
}

void IndexBufferData::build(const IndexArray::Pointer& i)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (_indexBuffer == 0)
	{
		glGenBuffers(1, &_indexBuffer);
		checkOpenGLError("glGenBuffers(1, &_indexBuffer)");
	}

	setProperties(i);
	internal_setData(i->data(), i->format() * _size);
#endif
}

void IndexBufferData::internal_setData(const unsigned char* data, size_t size)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (size == 0) return;
	
	_rc->renderState().bindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, drawTypeValue(_drawType));
	checkOpenGLError("glBufferData(GL_ELEMENT_ARRAY_BUFFER, %u, 0x%08X, ..,)", size, data);
#endif
}

void* IndexBufferData::indexOffset(size_t offset) const
{
	return reinterpret_cast<void*>(_format * offset);
}

void IndexBufferData::setData(const IndexArray::Pointer& i)
{
#if !defined(ET_CONSOLE_APPLICATION)
	build(i);
#endif
}

void IndexBufferData::overridePrimitiveType(PrimitiveType pt)
{
	_primitiveType = primitiveTypeValue(pt);
}
