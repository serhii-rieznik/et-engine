/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal_vertexbuffer.h>
#include <et/rendering/metal/metal.h>

namespace et
{

class MetalVertexBufferPrivate
{
public:
    MetalNativeBuffer buffer;
};

MetalVertexBuffer::MetalVertexBuffer(MetalState& metal, const VertexDeclaration& decl,
    const BinaryDataStorage& data, BufferDrawType drawType, const std::string& aName)
    : VertexBuffer(decl, drawType, aName)
{
	ET_PIMPL_INIT(MetalVertexBuffer);
    _private->buffer = MetalNativeBuffer(metal, data.data(), data.size());
}

MetalVertexBuffer::~MetalVertexBuffer()
{
	ET_PIMPL_FINALIZE(MetalVertexBuffer);
}

const MetalNativeBuffer& MetalVertexBuffer::nativeBuffer() const
{
    return _private->buffer;
}
    
void MetalVertexBuffer::bind()
{
    
}

void MetalVertexBuffer::setData(const void* data, size_t dataSize, bool invalidateExistingData)
{
    
}

void MetalVertexBuffer::setDataWithOffset(const void* data, size_t offset, size_t dataSize)
{
}

uint64_t MetalVertexBuffer::dataSize()
{
	return [_private->buffer.buffer() length];
}

void* MetalVertexBuffer::map(size_t offset, size_t dataSize, uint32_t options)
{
	return nullptr;
}

bool MetalVertexBuffer::mapped() const
{
	return false;
}

void MetalVertexBuffer::unmap()
{
}

void MetalVertexBuffer::clear()
{
    
}

}
