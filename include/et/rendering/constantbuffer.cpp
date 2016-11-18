/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/interface/renderer.h>
#include <et/rendering/constantbuffer.h>

namespace et
{
class ConstantBufferPrivate
{
public:
	HeapController heap;
	Buffer::Pointer buffer;
	BinaryDataStorage heapInfo;
	BinaryDataStorage localData;
	Vector<ConstantBufferEntry> allocations;
	bool modified = false;

	const ConstantBufferEntry& internalAlloc(uint32_t, bool);
	void internalFree(const ConstantBufferEntry&);
};

ConstantBuffer::ConstantBuffer()
{
	ET_PIMPL_INIT(ConstantBuffer);
}

ConstantBuffer::~ConstantBuffer()
{
	ET_PIMPL_FINALIZE(ConstantBuffer);
}

void ConstantBuffer::init(RenderInterface* renderer)
{
	_private->heap.init(Capacity, Granularity);
	_private->heapInfo.resize(_private->heap.requiredInfoSize());
	_private->heap.setInfoStorage(_private->heapInfo.begin());
	_private->heap.setAutoCompress(false);

	_private->buffer = renderer->createDataBuffer("shared-const-buffer", Capacity);
	_private->allocations.reserve(1024);
}

void ConstantBuffer::shutdown()
{
	_private->buffer.reset(nullptr);
	_private->heap.clear();
	_private->heapInfo.resize(0);
	_private->localData.resize(0);
}

Buffer::Pointer ConstantBuffer::buffer() const
{
	return _private->buffer;
}

void ConstantBuffer::flush()
{
	if (!_private->modified)
		return;

	uint8_t* mappedMemory = _private->buffer->map(0, Capacity);
	for (const ConstantBufferEntry& allocation : _private->allocations)
	{
		memcpy(mappedMemory + allocation.offset(), _private->localData.begin() + allocation.offset(), allocation.length());
		_private->buffer->modifyRange(allocation.offset(), allocation.length());
	}
	_private->buffer->unmap();

	for (const ConstantBufferEntry& allocation : _private->allocations)
	{
		if (allocation.isDynamic())
			free(allocation);
	}
	auto toErase = std::remove_if(_private->allocations.begin(), _private->allocations.end(), [](const ConstantBufferEntry& allocation){
		return allocation.isDynamic();
	});
	_private->allocations.erase(toErase, _private->allocations.end());
	_private->heap.compress();
	_private->modified = false;
}

const ConstantBufferEntry& ConstantBuffer::staticAllocate(uint32_t size)
{
	return _private->internalAlloc(size, false);
}

const ConstantBufferEntry& ConstantBuffer::dynamicAllocate(uint32_t size)
{
	return _private->internalAlloc(size, true);
}

void ConstantBuffer::free(const ConstantBufferEntry& entry)
{
	_private->internalFree(entry);

	if (entry.isDynamic() == false)
		_private->heap.compress();
}

const ConstantBufferEntry& ConstantBufferPrivate::internalAlloc(uint32_t size, bool dyn)
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
	allocations.emplace_back(offset, size, localData.begin() + offset, dyn);
	return allocations.back();
}

void ConstantBufferPrivate::internalFree(const ConstantBufferEntry& entry)
{
	ET_ASSERT(entry.data() >= localData.begin());
	ET_ASSERT(entry.data() < localData.end());
	
	if (heap.release(entry.offset()) == false)
		ET_ASSERT(!"Attempt to release memory which was not allocated here");

	if (entry.isDynamic() == false)
	{
		auto allocation = std::find(allocations.begin(), allocations.end(), entry);
		ET_ASSERT(allocation != allocations.end());
		allocations.erase(allocation);
	}
}

}
