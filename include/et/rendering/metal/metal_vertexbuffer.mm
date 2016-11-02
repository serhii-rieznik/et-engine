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
	std::atomic_bool mapped { false };
};

MetalVertexBuffer::MetalVertexBuffer(MetalState& metal, const VertexDeclaration& decl,
    const BinaryDataStorage& data, BufferDrawType drawType, const std::string& aName)
    : VertexBuffer(decl, drawType, aName)
{
	ET_PIMPL_INIT(MetalVertexBuffer);
    _private->buffer.construct(metal, data.data(), static_cast<uint32_t>(data.size()));

	setData(data.data(), 0, data.size());
}

MetalVertexBuffer::~MetalVertexBuffer()
{
	ET_PIMPL_FINALIZE(MetalVertexBuffer);
}

const MetalNativeBuffer& MetalVertexBuffer::nativeBuffer() const
{
    return _private->buffer;
}

void MetalVertexBuffer::setData(const void* data, uint32_t offset, uint32_t length)
{
	ET_ASSERT(offset + length <= dataSize());

	id<MTLBuffer> buf = _private->buffer.buffer();
	memcpy(reinterpret_cast<uint8_t*>([buf contents]) + offset, data, length);

	// TODO : update depending on draw type
	// [buf didModifyRange:NSMakeRange(offset, length)];
}

uint64_t MetalVertexBuffer::dataSize()
{
	return [_private->buffer.buffer() length];
}

void* MetalVertexBuffer::map(uint32_t offset, uint32_t length, uint32_t options)
{
	ET_ASSERT(offset + length <= dataSize());

	_private->mapped = true;
	uint8_t* data = reinterpret_cast<uint8_t*>([_private->buffer.buffer() contents]);
	return data + offset;
}

bool MetalVertexBuffer::mapped() const
{
	return _private->mapped;
}

void MetalVertexBuffer::unmap()
{
	ET_ASSERT(_private->mapped);
	_private->mapped = false;
}

}
