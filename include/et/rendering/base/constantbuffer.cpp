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
	HeapController heap;
	Buffer::Pointer buffer;
	BinaryDataStorage heapInfo;
	BinaryDataStorage localData;
	List<ConstantBufferEntry::Pointer> allocations;
	bool modified = false;

	const ConstantBufferEntry::Pointer& internalAlloc(uint32_t, bool);
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

void ConstantBuffer::init(RenderInterface* renderer)
{
	_private->heap.init(Capacity, Granularity);
	_private->heapInfo.resize(_private->heap.requiredInfoSize());
	_private->heap.setInfoStorage(_private->heapInfo.begin());
	_private->heap.setAutoCompress(false);

	_private->buffer = renderer->createDataBuffer("shared-const-buffer", Capacity);
}

void ConstantBuffer::shutdown()
{
	flush();
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
	if (_private->modified)
	{
		uint8_t* mappedMemory = _private->buffer->map(0, Capacity);
		for (const ConstantBufferEntry::Pointer& allocation : _private->allocations)
		{
			memcpy(mappedMemory + allocation->offset(), _private->localData.begin() + allocation->offset(), allocation->length());
			_private->buffer->modifyRange(allocation->offset(), allocation->length());
		}
		_private->buffer->unmap();
		_private->modified = false;
	}
	
	bool anyDeleted = false;
	for (auto i = _private->allocations.begin(); i != _private->allocations.end();)
	{
		if ((*i)->retainCount() == 1)
		{
			anyDeleted = true;
			_private->internalFree(*i);
			i = _private->allocations.erase(i);
		}
		else
		{
			++i;
		}
	}

	if (anyDeleted)
	{
		_private->heap.compress();
	}
}

const ConstantBufferEntry::Pointer& ConstantBuffer::staticAllocate(uint32_t size)
{
	return _private->internalAlloc(size, false);
}

const ConstantBufferEntry::Pointer& ConstantBuffer::dynamicAllocate(uint32_t size)
{
	return _private->internalAlloc(size, true);
}

const ConstantBufferEntry::Pointer& ConstantBufferPrivate::internalAlloc(uint32_t size, bool dyn)
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
	allocations.emplace_back(ConstantBufferEntry::Pointer::create(offset, size, localData.begin() + offset, dyn));
	return allocations.back();
}

void ConstantBufferPrivate::internalFree(const ConstantBufferEntry::Pointer& entry)
{
	ET_ASSERT(entry->data() >= localData.begin());
	ET_ASSERT(entry->data() < localData.end());
	
	if (heap.release(entry->offset()) == false)
		ET_ASSERT(0 && "Attempt to release memory which was not allocated here");

#if (ET_DEBUG)
	if (entry->isDynamic() == false)
	{
		auto allocation = std::find(allocations.begin(), allocations.end(), entry);
		ET_ASSERT(allocation == allocations.end());
	}
#endif
}

}
