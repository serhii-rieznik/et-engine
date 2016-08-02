/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal_IndexBuffer.h>

namespace et
{

class MetalIndexBufferPrivate
{
};

MetalIndexBuffer::MetalIndexBuffer(IndexArray::Pointer i, BufferDrawType dt, const std::string& name)
    : et::IndexBuffer(i, dt, name)
{
	ET_PIMPL_INIT(MetalIndexBuffer);
}

MetalIndexBuffer::~MetalIndexBuffer()
{
	ET_PIMPL_FINALIZE(MetalIndexBuffer);
}

void MetalIndexBuffer::bind()
{
}

void MetalIndexBuffer::clear()
{
}

}
