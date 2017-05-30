//
//  metal_databuffer.cpp
//  et-static-mac
//
//  Created by Sergey Reznik on 9/27/16.
//  Copyright © 2016 Cheetek. All rights reserved.
//

#include <et/rendering/metal/metal_buffer.h>
#include <et/rendering/metal/metal.h>

namespace et
{

class MetalBufferPrivate : public MetalNativeBuffer
{
public:
	std::atomic<bool> mapped{false};
};

MetalBuffer::MetalBuffer(MetalState& /* mtl */, const Description& /* desc */)
{
	ET_PIMPL_INIT(MetalBuffer);
	// _private->buffer = [mtl.device newBuffer]
}

MetalBuffer::~MetalBuffer()
{
	ET_PIMPL_FINALIZE(MetalBuffer);
}

uint8_t* MetalBuffer::map(uint32_t begin, uint32_t /* length */)
{
	uint8_t* ptr = reinterpret_cast<uint8_t*>([_private->buffer contents]) + begin;
	_private->mapped = true;
	return ptr;
}

void MetalBuffer::modifyRange(uint64_t /* begin */, uint64_t /* length */)
{

}

void MetalBuffer::unmap()
{
	_private->mapped = false;
}

bool MetalBuffer::mapped() const 
{
	return _private->mapped;
}

void MetalBuffer::updateData(uint32_t /* offset */, const BinaryDataStorage&)
{

}

void MetalBuffer::transferData(Buffer::Pointer /* destination */)
{

}

uint32_t MetalBuffer::size() const 
{
	return static_cast<uint32_t>([_private->buffer length]);
}

}

/*
class MetalBuffer : public Shared
{
public:
ET_DECLARE_POINTER(MetalNativeBuffer);

public:
MetalNativeBuffer() = default;
MetalNativeBuffer(const MetalNativeBuffer&) = delete;
MetalNativeBuffer(MetalNativeBuffer&&) = delete;
MetalNativeBuffer& operator = (const MetalNativeBuffer&) = delete;

MetalNativeBuffer(MetalState& metal, const void* data, uint32_t size)
{
construct(metal, data, size);
}

MetalNativeBuffer(MetalState& metal, uint32_t size)
{
construct(metal, size);
}

void construct(MetalState& metal, const void* data, uint32_t size)
{
_buffer = [metal.device newBufferWithBytes:data length:size options:MTLResourceCPUCacheModeDefaultCache];
}

void construct(MetalState& metal, uint32_t size)
{
_buffer = [metal.device newBufferWithLength:size options:MTLResourceStorageModeShared];
}

~MetalNativeBuffer()
{ ET_OBJC_RELEASE(_buffer); }

id<MTLBuffer> buffer() const
{ return _buffer; }

uint8_t* bufferContents() const
{ return reinterpret_cast<uint8_t*>([_buffer contents]); }

bool valid() const
{ return _buffer != nil; }

private:
id<MTLBuffer> _buffer = nil;
};
*/

