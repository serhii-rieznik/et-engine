/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
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
	build(i);
}

IndexBufferData::~IndexBufferData()
{
	if (_indexBuffer != 0)
		glDeleteBuffers(1, &_indexBuffer);

	_rc->renderState().indexBufferDeleted(_indexBuffer);
}

void IndexBufferData::setProperties(const IndexArray::Pointer& i)
{
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
			assert(false && "Invalud IndexArrayFormat value");
	}

	_primitiveType = primitiveTypeValue(i->primitiveType());
}

void IndexBufferData::build(const IndexArray::Pointer& i)
{
	if (_indexBuffer == 0)
	{
		glGenBuffers(1, &_indexBuffer);
		checkOpenGLError("glGenBuffers(1, &_indexBuffer)");
	}

	setProperties(i);
	internal_setData(i->data(), i->format() * _size);
}

void IndexBufferData::internal_setData(const unsigned char* data, size_t size)
{
	if (size > 0)
	{
		_rc->renderState().bindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, drawTypeValue(_drawType));
		checkOpenGLError("glBufferData(GL_ELEMENT_ARRAY_BUFFER, %u, 0x%08X, ..,)", size, data);
	}
}

void* IndexBufferData::indexOffset(size_t offset) const
{
	return reinterpret_cast<void*>(_format * offset);
}

void IndexBufferData::setData(const IndexArray::Pointer& i)
{
	build(i);
}

void IndexBufferData::overridePrimitiveType(PrimitiveType pt)
{
	_primitiveType = primitiveTypeValue(pt);
	
}
