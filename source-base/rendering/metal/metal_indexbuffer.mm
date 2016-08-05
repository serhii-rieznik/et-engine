/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal_indexbuffer.h>
#include <et/rendering/metal/metal.h>

namespace et
{

class MetalIndexBufferPrivate
{
public:
	MetalBuffer buffer;
};

MetalIndexBuffer::MetalIndexBuffer(MetalState& metal, IndexArray::Pointer i, BufferDrawType dt, const std::string& name)
    : et::IndexBuffer(i, dt, name)
{
	ET_PIMPL_INIT(MetalIndexBuffer);
	_private->buffer = MetalBuffer(metal, i->data(), i->actualSize());
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
