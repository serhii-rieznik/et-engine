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
	BinaryDataStorage heapInfo;
	BinaryDataStorage localData;
	DataBuffer::Pointer buffer;
	
	Vector<ConstantBufferEntry> dynamicAllocations;
	bool modified = true;
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
	
	_private->localData.resize(Capacity);
	_private->localData.fill(0);

	_private->buffer = renderer->createDataBuffer("shared-const-buffer", Capacity);
	_private->dynamicAllocations.reserve(1024);
}

void ConstantBuffer::shutdown()
{
	_private->buffer.reset(nullptr);
	_private->heap.clear();
	_private->localData.resize(0);
	_private->heapInfo.resize(0);
}

DataBuffer::Pointer ConstantBuffer::buffer() const
{
	return _private->buffer;
}

void ConstantBuffer::flush()
{
	if (_private->modified)
	{
		_private->buffer->setData(_private->localData.begin(), 0, _private->localData.size());
		for (ConstantBufferEntry allocation : _private->dynamicAllocations)
		{
			free(allocation);
		}
		_private->dynamicAllocations.clear();
		_private->modified = false;
	}
	else
	{
		ET_ASSERT(_private->dynamicAllocations.empty());
	}
}

ConstantBufferEntry ConstantBuffer::staticAllocate(uint32_t size)
{
	uint32_t offset = 0;
	
	if (!_private->heap.allocate(size, offset))
		ET_FAIL("Failed to allocate data in shared constant buffer")

	_private->modified = true;	
	return ConstantBufferEntry(offset, size, _private->localData.begin() + offset);
}

ConstantBufferEntry ConstantBuffer::dynamicAllocate(uint32_t size)
{
	_private->dynamicAllocations.emplace_back(staticAllocate(size));
	return _private->dynamicAllocations.back();
}

void ConstantBuffer::free(const ConstantBufferEntry& entry)
{
	ET_ASSERT(entry.data() >= _private->localData.begin());
	ET_ASSERT(entry.data() < _private->localData.end());

	if (_private->heap.release(entry.offset()) == false)
	{
		ET_FAIL_FMT("Attempt to release memory which was not allocated here");
	}
}

}
