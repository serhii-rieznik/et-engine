/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/opengl/opengl.h>
#include <et/rendering/rendercontext.h>
#include <et/apiobjects/indexbufferdata.h>

using namespace et;

IndexBufferData::IndexBufferData(RenderContext* rc, IndexArray::Pointer i, BufferDrawType drawType,
	const std::string& aName) : Object(aName), _rc(rc), _size(i->actualSize()), _sourceTag(0),
	_indexBuffer(0), _drawType(drawType)
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
	_primitiveType = i->primitiveType();
	_format = i->format();

	switch (_format)
	{
		case IndexArrayFormat::Format_8bit:
		{
			_dataType = DataType::UnsignedChar;
			break;
		}
			
		case IndexArrayFormat::Format_16bit:
		{
			_dataType = DataType::UnsignedShort;
			break;
		}
			
		case IndexArrayFormat::Format_32bit:
		{
			_dataType = DataType::UnsignedInt;
			break;
		}
			
		default:
			ET_FAIL_FMT("Invalid IndexArrayFormat value: %u", static_cast<uint32_t>(_format));
	}
	
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
	internal_setData(i->data(), static_cast<size_t>(i->format()) * _size);
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
	return reinterpret_cast<void*>(static_cast<size_t>(_format) * offset);
}

void IndexBufferData::setData(const IndexArray::Pointer& i)
{
#if !defined(ET_CONSOLE_APPLICATION)
	build(i);
#endif
}

void IndexBufferData::overridePrimitiveType(PrimitiveType pt)
{
	_primitiveType = pt;
}
