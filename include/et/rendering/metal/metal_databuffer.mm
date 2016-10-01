//
//  metal_databuffer.cpp
//  et-static-mac
//
//  Created by Sergey Reznik on 9/27/16.
//  Copyright © 2016 Cheetek. All rights reserved.
//

#include <et/rendering/metal/metal_databuffer.h>
#include <et/rendering/metal/metal.h>

namespace et
{

class MetalDataBufferPrivate
{
public:
	MetalDataBufferPrivate(MetalState& metal, const BinaryDataStorage& data) :
		buffer(metal, data.data(), data.size()) { }

	MetalDataBufferPrivate(MetalState& metal, uint32_t size) :
		buffer(metal, size), writable(true) { }

	MetalNativeBuffer buffer;
	bool writable = false;
};

MetalDataBuffer::MetalDataBuffer(MetalState& metal, const BinaryDataStorage& data)
{
	ET_PIMPL_INIT(MetalDataBuffer, metal, data)
}

MetalDataBuffer::MetalDataBuffer(MetalState& metal, uint32_t size)
{
	ET_PIMPL_INIT(MetalDataBuffer, metal, size)
}

MetalDataBuffer::~MetalDataBuffer()
{
	ET_PIMPL_FINALIZE(MetalDataBuffer);
}

uint32_t MetalDataBuffer::size() const
{
	return static_cast<uint32_t>([_private->buffer.buffer() length]);
}

void MetalDataBuffer::setData(const void* src, uint32_t offset, uint32_t sz)
{
	ET_ASSERT(_private->writable);
	ET_ASSERT(offset + sz <= size());
	memcpy(_private->buffer.bufferContents() + offset, src, sz);
}

const MetalNativeBuffer& MetalDataBuffer::nativeBuffer() const
{
	return _private->buffer;
}

}
