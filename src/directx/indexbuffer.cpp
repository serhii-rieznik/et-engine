/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_WIN && ET_DIRECTX_RENDER)

using namespace et;

IndexBufferData::IndexBufferData(RenderContext* rc, IndexArray::Pointer i, BufferDrawType drawType,
	const std::string& aName) : APIObject(aName), _rc(rc), _size(i->actualSize()), _sourceTag(0), _drawType(drawType)
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
	uint32_t buffer = static_cast<uint32_t>(apiHandle());
	if (buffer != 0)
	{
		_rc->renderState().indexBufferDeleted(buffer);
	}
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
	if (apiHandleInvalid())
	{
		setAPIHandle(0);
	}

	setProperties(i);
	internal_setData(i->data(), static_cast<size_t>(i->format()) * _size);
#endif
}

void IndexBufferData::internal_setData(const unsigned char* data, size_t size)
{
#if !defined(ET_CONSOLE_APPLICATION)
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

#endif // ET_PLATFORM_WIN && ET_DIRECTX_RENDER
