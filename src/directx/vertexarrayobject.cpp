/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_WIN && ET_DIRECTX_RENDER)

using namespace et;

VertexArrayObjectData::VertexArrayObjectData(RenderContext* rc, VertexBuffer::Pointer vb, IndexBuffer ib,
	const std::string& aName) : APIObject(aName), _rc(rc), _vb(vb), _ib(ib)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create VertexArrayObject in console application.")
#else
	init();
#endif
}

VertexArrayObjectData::VertexArrayObjectData(RenderContext* rc, const std::string& aName) :
	APIObject(aName), _rc(rc)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create VertexArrayObject in console application.")
#else
	init();
#endif
}

VertexArrayObjectData::~VertexArrayObjectData()
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void VertexArrayObjectData::init()
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void VertexArrayObjectData::setBuffers(VertexBuffer::Pointer vb, IndexBuffer ib)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void VertexArrayObjectData::setVertexBuffer(VertexBuffer::Pointer vb)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void VertexArrayObjectData::setIndexBuffer(IndexBuffer ib)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

#endif // ET_PLATFORM_WIN && ET_DIRECTX_RENDER
