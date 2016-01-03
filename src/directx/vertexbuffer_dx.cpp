/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_WIN && ET_DIRECTX_RENDER)

using namespace et;

VertexBuffer::VertexBuffer(RenderContext* rc, const VertexArray::Description& desc,
	BufferDrawType vertexDrawType, const std::string& aName) : APIObject(aName), _rc(rc),
	_decl(desc.declaration), _drawType(vertexDrawType)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create VertexBuffer in console application.")
#else
	setAPIHandle(0);
	setData(desc.data.data(), desc.data.dataSize());
#endif
}

VertexBuffer::VertexBuffer(RenderContext* rc, const VertexDeclaration& decl, const void* vertexData,
	size_t vertexDataSize, BufferDrawType vertexDrawType, const std::string& aName) : APIObject(aName), _rc(rc),
	_decl(decl), _drawType(vertexDrawType)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create VertexBuffer in console application.")
#else
	setAPIHandle(0);
	setData(vertexData, vertexDataSize);
#endif
}

VertexBuffer::~VertexBuffer()
{
#if !defined(ET_CONSOLE_APPLICATION)
	uint32_t buffer = static_cast<uint32_t>(apiHandle());
	if (buffer != 0)
	{
		_rc->renderState().vertexBufferDeleted(buffer);
	}
#endif
}

void VertexBuffer::setData(const void* data, size_t dataSize)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_dataSize = dataSize;
#endif
}

void* VertexBuffer::map(size_t offset, size_t dataSize, MapBufferMode mode)
{
	ET_ASSERT(!_mapped)
	ET_ASSERT(dataSize > 0)

	void* result = nullptr;
	_mapped = true;
	return result;
}

void VertexBuffer::unmap()
{
#if !defined(ET_CONSOLE_APPLICATION)
	_mapped = false;
#endif
}

void VertexBuffer::serialize(std::ostream&)
{
	ET_FAIL("Unsupported");
}

void VertexBuffer::deserialize(std::istream&)
{
	ET_FAIL("Unsupported");
}

#endif // ET_PLATFORM_WIN && ET_DIRECTX_RENDER
