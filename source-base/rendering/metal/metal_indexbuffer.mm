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
	MetalNativeBuffer buffer;
};

MetalIndexBuffer::MetalIndexBuffer(MetalState& metal, IndexArray::Pointer i, BufferDrawType dt, const std::string& name)
    : et::IndexBuffer(i, dt, name)
{
	ET_PIMPL_INIT(MetalIndexBuffer);
    uint32_t actualDataSize = i->actualSize() * static_cast<uint32_t>(i->format());
	_private->buffer.construct(metal, i->data(), actualDataSize);
}

MetalIndexBuffer::~MetalIndexBuffer()
{
	ET_PIMPL_FINALIZE(MetalIndexBuffer);
}
    
const MetalNativeBuffer& MetalIndexBuffer::nativeBuffer() const
{
    return _private->buffer;
}

void MetalIndexBuffer::bind()
{

}

}
