/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/interface/renderer.h>
#include <et/rendering/base/constantbuffer.h>

namespace et
{
class ConstantBufferPrivate
{
public:
	RemoteHeap heap;
	Buffer::Pointer buffer;
	BinaryDataStorage heapInfo;
	BinaryDataStorage localData;
	List<ConstantBufferEntry::Pointer> allocations;
	uint32_t allowedAllocations = 0;
	bool modified = false;

	const ConstantBufferEntry::Pointer& allocateInternal(uint32_t, uint32_t);
	void internalFree(const ConstantBufferEntry::Pointer&);
};

ConstantBuffer::ConstantBuffer()
{
	ET_PIMPL_INIT(ConstantBuffer);
}

ConstantBuffer::~ConstantBuffer()
{
	ET_PIMPL_FINALIZE(ConstantBuffer);
}

void ConstantBuffer::init(RenderInterface* renderer, uint32_t allowedAllocations)
{
	_private->allowedAllocations = allowedAllocations;

	_private->heap.init(Capacity, Granularity);
	_private->heapInfo.resize(_private->heap.requiredInfoSize());
	_private->heap.setInfoStorage(_private->heapInfo.begin());

	_private->buffer = renderer->createDataBuffer("shared-const-buffer", Capacity);
}

void ConstantBuffer::shutdown()
{
	_private->heap.clear();
	_private->buffer.reset(nullptr);
	_private->heapInfo.resize(0);
	_private->localData.resize(0);
}

Buffer::Pointer ConstantBuffer::buffer() const
{
	return _private->buffer;
}

void ConstantBuffer::flush(uint32_t frameNumber)
{
	if (_private->modified)
	{
		uint8_t* mappedMemory = _private->buffer->map(0, Capacity);
		for (ConstantBufferEntry::Pointer& allocation : _private->allocations)
		{
			if (allocation->flushFrame() == InvalidFlushFrame)
			{
				memcpy(mappedMemory + allocation->offset(), _private->localData.begin() + allocation->offset(), allocation->length());
				_private->buffer->modifyRange(allocation->offset(), allocation->length());
				allocation->flush(frameNumber);
			}
		}
		_private->buffer->unmap();
		_private->modified = false;
	}
	
	auto i = std::remove_if(_private->allocations.begin(), _private->allocations.end(), [this, frameNumber](const ConstantBufferEntry::Pointer& e)
	{
		bool shouldRelease = (e->retainCount() == 1) /* && (frameNumber > e->flushFrame() + RendererFrameCount)*/;
		
		if (shouldRelease)
			_private->internalFree(e);

		return shouldRelease;
	});
	
	if (i != _private->allocations.end())
		_private->allocations.erase(i, _private->allocations.end());
}

const ConstantBufferEntry::Pointer& ConstantBuffer::allocate(uint32_t size, uint32_t allocationClass)
{
	ET_ASSERT(_private->allowedAllocations & allocationClass);
	return _private->allocateInternal(size, allocationClass);
}

const ConstantBufferEntry::Pointer& ConstantBufferPrivate::allocateInternal(uint32_t size, uint32_t cls)
{
	uint32_t offset = 0;

	if (!heap.allocate(size, offset))
		ET_FAIL("Failed to allocate data in shared constant buffer");

	if (localData.empty())
	{
		localData.resize(ConstantBuffer::Capacity);
		localData.fill(0);
	}

	modified = true;
	allocations.emplace_back(ConstantBufferEntry::Pointer::create(offset, size, localData.begin() + offset, cls));
	return allocations.back();
}

void ConstantBufferPrivate::internalFree(const ConstantBufferEntry::Pointer& entry)
{
	ET_ASSERT(entry->data() >= localData.begin());
	ET_ASSERT(entry->data() < localData.end());
	
	if (heap.release(entry->offset()) == false)
		ET_ASSERT(!"Attempt to release memory which was not allocated here");
}

}
