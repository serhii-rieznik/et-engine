/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>

namespace et
{

struct RemoteHeapPrivate
{
	enum : uint8_t
	{
		Empty = 0x00,
		AllocationBegin = 0x01,
		AllocationInterior = 0x02,
		AllocationEnd = 0x03
	};

	uint32_t capacity = 0;
	uint32_t granularity = 0;
	uint32_t infoSize = 0;
	uint32_t firstEmpty = 0;
	uint32_t allocatedSize = 0;
	uint8_t* info = nullptr;
};

RemoteHeap::RemoteHeap()
{
	ET_PIMPL_INIT(RemoteHeap);
}

RemoteHeap::RemoteHeap(uint32_t cap, uint32_t gr)
{
	ET_PIMPL_INIT(RemoteHeap);
	init(cap, gr);
}

RemoteHeap::RemoteHeap(RemoteHeap&& r)
{
	ET_PIMPL_INIT(RemoteHeap);
	std::swap(*_private, *r._private);
}

RemoteHeap& RemoteHeap::operator = (RemoteHeap&& r)
{
	std::swap(*_private, *r._private);
	r._private->capacity = 0;
	r._private->granularity = 0;
	r._private->infoSize = 0;
	r._private->firstEmpty = 0;
	r._private->allocatedSize = 0;
	r._private->info = nullptr;
	return *this;
}

RemoteHeap::~RemoteHeap()
{
	ET_PIMPL_FINALIZE(RemoteHeap);
}

void RemoteHeap::init(uint32_t cap, uint32_t gr)
{
	_private->capacity = cap;
	_private->granularity = gr;
	_private->infoSize = (_private->capacity + _private->granularity - 1) / _private->granularity;
}

bool RemoteHeap::allocate(uint32_t sizeToAllocate, uint32_t& offset)
{
	uint32_t alignedSize = alignUpTo(sizeToAllocate, _private->granularity);
	uint32_t requiredInfoSize = alignedSize / _private->granularity;

	if (_private->firstEmpty + requiredInfoSize > _private->infoSize)
		return false;

	bool allocated = false;

	uint8_t* ptr = _private->info + _private->firstEmpty;
	uint8_t* end = _private->info + _private->infoSize;

	ET_ASSERT(*ptr == RemoteHeapPrivate::Empty);
	
	uint32_t sz = 1;
	uint8_t* seekBegin = ptr;
	uint8_t* allocationBegin = ptr;
	while (ptr < end)
	{
		if (*ptr)
		{
			allocationBegin += sz;
			sz = 0;
		}

		if (sz == requiredInfoSize)
		{
			ET_ASSERT(*allocationBegin == RemoteHeapPrivate::Empty);

			uint8_t* allocationEnd = allocationBegin + requiredInfoSize - 1;
			ET_ASSERT(*allocationEnd == RemoteHeapPrivate::Empty);
			
			offset = _private->granularity * static_cast<uint32_t>(allocationBegin - _private->info);
			_private->allocatedSize += _private->granularity * requiredInfoSize;

			uint32_t usedChunks = static_cast<uint32_t>(allocationEnd - allocationBegin);
			memset(allocationBegin, RemoteHeapPrivate::AllocationInterior, usedChunks);
			*allocationBegin = RemoteHeapPrivate::AllocationBegin;
			*allocationEnd = RemoteHeapPrivate::AllocationEnd;
			
			if (allocationBegin == seekBegin)
			{
				_private->firstEmpty += usedChunks;
				while ((allocationEnd < end) && (*allocationEnd++))
					_private->firstEmpty++;
			}
			allocated = true;
			break;
		}

		++ptr;
		++sz;
	}
	
	return allocated;
}

bool RemoteHeap::release(uint32_t offset)
{
	if (offset % _private->granularity)
		return false;

	uint32_t index = offset / _private->granularity;
	if (index >= _private->infoSize)
		return false;

	uint8_t* ptr = _private->info + index;
	uint8_t state = *ptr;
	ET_ASSERT((state == RemoteHeapPrivate::AllocationBegin) || (state == RemoteHeapPrivate::AllocationEnd));

	uint32_t freedSize = 0;
	do 
	{
		freedSize++;
		state = *ptr;
		*ptr++ = RemoteHeapPrivate::Empty;
	} while (state != RemoteHeapPrivate::AllocationEnd);

	freedSize *= _private->granularity;
	ET_ASSERT(freedSize <= _private->allocatedSize);

	_private->allocatedSize -= freedSize;
	_private->firstEmpty = std::min(_private->firstEmpty, index);
	ET_ASSERT(_private->info[_private->firstEmpty] == RemoteHeapPrivate::Empty);

	return true;
}

bool RemoteHeap::containsAllocationWithOffset(uint32_t offset)
{
	if (offset % _private->granularity != 0)
		return false;

	uint32_t index = offset / _private->granularity;
	if (index >= _private->infoSize)
		return false;

	return true;
}

uint32_t RemoteHeap::requiredInfoSize() const
{
	return _private->infoSize;
}

uint32_t RemoteHeap::capacity() const
{
	return _private->capacity;
}

void RemoteHeap::setInfoStorage(void* ptr)
{
	_private->info = reinterpret_cast<uint8_t*>(ptr);
	memset(_private->info, RemoteHeapPrivate::Empty, _private->infoSize);
}

bool RemoteHeap::empty() const
{
	return allocatedSize() == 0;
}

uint32_t RemoteHeap::allocatedSize() const
{
	return _private->allocatedSize;
}

void RemoteHeap::clear()
{
	_private->allocatedSize = 0;
	_private->firstEmpty = 0;
	memset(_private->info, RemoteHeapPrivate::Empty, _private->infoSize);
}

}
