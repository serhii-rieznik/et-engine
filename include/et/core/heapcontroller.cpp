/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>
#include "heapcontroller.h"

#define ET_HEAP_USE_FIRST_FREE 0
#define ET_HEAP_DEEP_DEBUG     0

namespace et
{
struct HeapChunkInfo
{
	union
	{
		struct
		{
			uint32_t allocationMask;
			uint32_t begin;
			uint32_t length;
			uint32_t allocIndex;
		};
		struct
		{
			uint64_t v0;
			uint64_t v1;
		};
	};

	void swapWith(HeapChunkInfo* info)
	{
		auto& iv0 = info->v0;
		auto& iv1 = info->v1;
		v0 ^= iv0;
		iv0 ^= v0;
		v0 ^= iv0;
		v1 ^= iv1;
		iv1 ^= v1;
		v1 ^= iv1;
	}

	enum : uint32_t
	{
		Available = 0xFFFFFFFF,
		Used = 0x00000000
	};
};

class HeapControllerPrivate
{
public:
#if (ET_HEAP_USE_FIRST_FREE)
	uint64_t firstFreeChunk = 0;
#endif
	
	uint32_t capacity = 0;
	uint32_t granularity = 1;
	uint32_t maxInfoChunks = 0;
	HeapChunkInfo* infoStorage = nullptr;
	HeapChunkInfo* firstInfo = nullptr;
	HeapChunkInfo* lastInfo = nullptr;
	bool autoCompress = true;

	void validateInfo(HeapChunkInfo* info);
	void compress();
};

HeapController::HeapController()
{
	ET_PIMPL_INIT(HeapController);
}

HeapController::HeapController(uint32_t cap, uint32_t gr)
{
	ET_PIMPL_INIT(HeapController);
	init(cap, gr);
}

HeapController::~HeapController()
{
	ET_PIMPL_FINALIZE(HeapController);
}

void HeapController::init(uint32_t cap, uint32_t gr)
{
	_private->capacity = cap;
	_private->granularity = gr;
	
	ET_ASSERT(_private->capacity > 0);
	ET_ASSERT(_private->granularity > 0);
	_private->maxInfoChunks = (_private->capacity + _private->granularity - 1) / (_private->granularity);
}

extern std::set<uint32_t> _breakOnAllocations;

bool HeapController::allocate(uint32_t sizeToAllocate, uint32_t& offset)
{
	ET_ASSERT(_private->capacity > 0);
	ET_ASSERT(_private->infoStorage != nullptr);

	sizeToAllocate = alignUpTo(sizeToAllocate, _private->granularity);

	HeapChunkInfo* info = _private->firstInfo;

#if (ET_HEAP_USE_FIRST_FREE)
	info += _private->firstFreeChunk;
#endif
	while (info < _private->lastInfo)
	{
		_private->validateInfo(info);
		if ((info->allocationMask & info->length) >= sizeToAllocate)
		{
			uint32_t remaining = info->length - sizeToAllocate;

			info->allocationMask = HeapChunkInfo::Used;
			info->length = sizeToAllocate;

#		if ET_DEBUG
			info->allocIndex = BlockMemoryAllocator::allocationIndex++;
			if (_breakOnAllocations.count(info->allocIndex))
			{
				debug::debugBreak();
			}
#		endif

			if (remaining > _private->granularity)
			{
				HeapChunkInfo* nextInfo = info + 1;

				if (nextInfo >= _private->lastInfo) // last one reached
				{
					_private->lastInfo = nextInfo + 1;
				}
				else
				{
					HeapChunkInfo* i = _private->lastInfo;
					HeapChunkInfo* prev = i - 1;
					while (i > nextInfo)
					{
						prev->swapWith(i);
						--prev;
						--i;
					}
					++_private->lastInfo;
				}

				nextInfo->allocationMask = HeapChunkInfo::Available;
				nextInfo->begin = info->begin + info->length;
				nextInfo->length = remaining;
			#if (ET_HEAP_USE_FIRST_FREE)
				_private->firstFreeChunk = std::min(_private->firstFreeChunk, static_cast<uint64_t>(nextInfo - _private->firstInfo));
			#endif
			}

			offset = info->begin;
			return true;
		}
		++info;
	}

	return false;
}

bool HeapController::release(uint32_t offset)
{
	HeapChunkInfo* i = _private->firstInfo;
	while (i < _private->lastInfo)
	{
		_private->validateInfo(i);
		if (i->begin == offset)
		{
			if (i->allocationMask & HeapChunkInfo::Available)
			{
				ET_FAIL_FMT("Pointer being freed (0x%08x) was already deleted from this memory chunk.", offset);
				return false;
			}
			else
			{
			#if (ET_HEAP_USE_FIRST_FREE)
				_private->firstFreeChunk = std::min(_private->firstFreeChunk, static_cast<uint64_t>(i - _private->firstInfo));
			#endif
				i->allocationMask = HeapChunkInfo::Available;
				if (_private->autoCompress)
					_private->compress();
				return true;
			}
		}
		++i;
	}

	return false;
}

bool HeapController::containsAllocationWithOffset(uint32_t offset)
{
	HeapChunkInfo* i = _private->firstInfo;
	while (i < _private->lastInfo)
	{
		if (i->begin == offset)
			return true;
		++i;
	}
	return false;
}

uint32_t HeapController::requiredInfoSize() const
{
	uint32_t sz = static_cast<uint32_t>(_private->maxInfoChunks * sizeof(HeapChunkInfo));
	return alignUpTo(sz, _private->granularity);
}

uint32_t HeapController::capacity() const
{
	return _private->capacity;
}

void HeapController::setInfoStorage(void* ptr)
{
	_private->infoStorage = reinterpret_cast<HeapChunkInfo*>(ptr);
	HeapChunkInfo* lastEntry = _private->infoStorage + _private->maxInfoChunks;
	for (HeapChunkInfo* i = _private->infoStorage; i < lastEntry; ++i)
	{
		i->allocationMask = HeapChunkInfo::Available;
		i->begin = 0;
		i->length = 0;
		i->allocIndex = 0;
	}
	
	_private->firstInfo = _private->infoStorage;
	_private->firstInfo->length = _private->capacity;
	_private->lastInfo = _private->firstInfo + 1;
}

void HeapController::setAutoCompress(bool value)
{
	_private->autoCompress = value;
	
	if (value)
		_private->compress();
}

bool HeapController::empty() const
{
	for (HeapChunkInfo* i = _private->firstInfo; i < _private->lastInfo; ++i)
	{
		if (i->allocationMask == HeapChunkInfo::Used)
			return false;
	}
	return true;
}

void HeapController::compress()
{
	_private->compress();
}

void HeapController::clear()
{
	_private->capacity = 0;
	_private->granularity = 0;
	_private->maxInfoChunks = 0;
	_private->firstInfo = nullptr;
	_private->lastInfo = nullptr;
	_private->infoStorage = nullptr;
}

uint32_t HeapController::currentlyAllocatedSize() const
{
	uint32_t result = 0;
	for (HeapChunkInfo* i = _private->firstInfo; i < _private->lastInfo; ++i)
		result += i->length & (~i->allocationMask);
	return result;
}

void HeapController::getAllocationIndexes(std::vector<uint32_t>& indexes) const
{
	for (HeapChunkInfo* i = _private->firstInfo; i < _private->lastInfo; ++i)
	{
		if (i->allocationMask == HeapChunkInfo::Used)
			indexes.emplace_back(i->allocIndex);
	}
}

void HeapControllerPrivate::compress()
{
	HeapChunkInfo* i = firstInfo;

#if (ET_HEAP_USE_FIRST_FREE)
	i += firstFreeChunk;
#endif
	
	while (i < lastInfo)
	{
		validateInfo(i);
		if (i->allocationMask & HeapChunkInfo::Available)
		{
			HeapChunkInfo* nextInfo = i + 1;
			if ((nextInfo < lastInfo) && (nextInfo->allocationMask & HeapChunkInfo::Available))
			{
				i->length += nextInfo->length;

				HeapChunkInfo* next = nextInfo + 1;
				while (nextInfo < lastInfo)
					(nextInfo++)->swapWith(next++);

				--lastInfo;
			}
			else
			{
				++i;
			}
		}
		else
		{
			++i;
		}
	}
}

void HeapControllerPrivate::validateInfo(HeapChunkInfo* info)
{
#if (ET_HEAP_DEEP_DEBUG)
	ET_ASSERT((info->allocationMask == HeapChunkInfo::Used) || (info->allocationMask == HeapChunkInfo::Available));
	ET_ASSERT(info->begin < capacity);
	ET_ASSERT(info->length <= capacity);
	ET_ASSERT(info->begin + info->length <= capacity);
#endif
	(void)info;
}


}
