/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/criticalsection.h>
#include <et/core/staticdatastorage.h>

namespace et
{
#if (ET_DEBUG)
std::atomic<uint32_t> MemoryAllocatorBase::allocationIndex(0);
static std::set<uint32_t> _breakOnAllocations;
void MemoryAllocatorBase::allocateOnBreaks(const std::set<uint32_t>& indices)
{
	_breakOnAllocations.insert(indices.begin(), indices.end());
}
#endif

enum : uint32_t
{
	
#	if (ET_DEBUG)
	minimumAllocationStatisticsSize = 96,
	maximumAllocationStatisticsSize = (uint32_t)((2048 + minimumAllocationStatisticsSize) / minimumAllocationStatisticsSize),
#	endif

	megabytes = 1024 * 1024,
	allocatedValue = 0x00000000,
	notAllocatedValue = 0xffffffff,
	defaultChunkSize = 16 * megabytes,
	minimumAllocationSize = 32,
	smallBlockSize = 60,
	mediumBlockSize = 124,
};

struct MemoryChunkInfo
{
	union
	{
		struct
		{
			uint32_t allocated;
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

	void swapWith(MemoryChunkInfo* info)
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
};

class MemoryChunk
{
public:
	MemoryChunk(uint32_t);
	
	~MemoryChunk();
	
	bool allocate(uint32_t size, void*& result);
	bool free(char*);
	
	bool containsPointer(char*);
	
	void compress();
	
#	if (ET_DEBUG)
	void setBreakOnAllocation()
		{ breakOnAllocation = true; }
#	endif
	
private:
	inline void validateInfo(MemoryChunkInfo*);
	
private:
	MemoryChunk(const MemoryChunk&) = delete;
	MemoryChunk& operator = (const BlockMemoryAllocator&) = delete;
	
public:
	uint32_t compressCounter = 0;
	uint32_t size = 0;
	uint32_t actualDataOffset = 0;
	uint32_t index = 0;
	
	MemoryChunkInfo* firstInfo = nullptr;
	MemoryChunkInfo* lastInfo = nullptr;
	
	char* allocatedMemoryBegin = nullptr;
	char* allocatedMemoryEnd = nullptr;
	
#if (ET_DEBUG)
	StaticDataStorage<uint32_t, maximumAllocationStatisticsSize> allocationStatistics;
	StaticDataStorage<uint32_t, maximumAllocationStatisticsSize> deallocationStatistics;
	bool breakOnAllocation = false;
#endif
};


template <int blockSize>
class SmallMemoryBlockAllocator
{
public:
	enum
	{
		BlockSize = blockSize
	};
	
	SmallMemoryBlockAllocator()
	{
		auto sizeToAllocate = blocksCount * sizeof(SmallMemoryBlock);
		blocks = reinterpret_cast<SmallMemoryBlock*>(calloc(1, sizeToAllocate));
		firstBlock = blocks;
		currentBlock = firstBlock;
		lastBlock = firstBlock + blocksCount;
	}
	
	~SmallMemoryBlockAllocator()
	{
	#if (ET_DEBUG)
		std::vector<uint32_t> detectedLeaks;
		detectedLeaks.reserve(1024);
		uint32_t totalLeaked = 0;

		log::ConsoleOutput lOut;
		for (auto i = firstBlock; i != lastBlock; ++i)
		{
			if (i->blockAllocated)
			{
				totalLeaked += blockSize;
				detectedLeaks.emplace_back(i->allocIndex);
			}
		}

		if (totalLeaked > 0)
		{
			char buffer[1024 * 10] = { };

			int printPos = 0;
			for (uint32_t i : detectedLeaks)
				printPos += sprintf(buffer + printPos, "%u, ", i);
			buffer[printPos - 2] = 0;

			lOut.info("Total memory leaked: %llu from small blocks, use debug funtion:", totalLeaked);
			lOut.info("et::MemoryAllocatorBase::allocateOnBreaks({ %s })", buffer);
		}
	#endif
		::free(blocks);
	}
	
	bool allocate(void*& result)
	{
		auto startBlock = currentBlock;
		
		while (currentBlock->blockAllocated)
		{
			currentBlock = advance(currentBlock);
			if (currentBlock == startBlock)
			{
				_haveFreeBlocks = false;
				log::warning("Small memory block (%d) filled.", blockSize);
				return false;
			}
		}
		
		currentBlock->blockAllocated = 1;
#	if (ET_DEBUG)
		currentBlock->allocIndex = MemoryAllocatorBase::allocationIndex++;
		if (_breakOnAllocations.count(currentBlock->allocIndex))
		{
			debug::debugBreak();
		}
#	endif
		result = currentBlock->data;
		currentBlock = advance(currentBlock);
		
		return true;
	}
	
	void free(void* ptr)
	{
		SmallMemoryBlock* block = reinterpret_cast<SmallMemoryBlock*>(ptr);
		
		if ((ptr < firstBlock) || (ptr > lastBlock))
			ET_FAIL_FMT("Pointer being freed (0x%016llx) was not allocated via this allocator.", (int64_t)ptr);
		
		block->blockAllocated = 0;
		
		if (!_haveFreeBlocks)
			currentBlock = block;

		_haveFreeBlocks = true;
	}
	
	bool containsPointer(void* ptr)
	{
		SmallMemoryBlock* block = reinterpret_cast<SmallMemoryBlock*>(ptr);
		return (block >= firstBlock) && (block < lastBlock);
	}
	
	bool haveFreeBlocks()
		{ return _haveFreeBlocks; }
	
private:
	enum
	{
		blocksCount = 8 * megabytes / (blockSize + 4)
	};
	
	struct SmallMemoryBlock
	{
		static_assert((blockSize + sizeof(uint32_t)) % 32 == 0,
			"Invalid block size will cause troubles allocating aligned objects");
		
		char data[blockSize];
		uint32_t blockAllocated = 0;
#	if (ET_DEBUG)
		uint32_t allocIndex = 0;
#	endif
	};
	
	SmallMemoryBlock* advance(SmallMemoryBlock* b)
	{
		++b;
		return (b == lastBlock) ? firstBlock : b;
	}
	
public:
	SmallMemoryBlock* blocks = nullptr;
	SmallMemoryBlock* firstBlock = nullptr;
	SmallMemoryBlock* lastBlock = nullptr;
	SmallMemoryBlock* currentBlock = nullptr;
	bool _haveFreeBlocks = true;
};

class BlockMemoryAllocatorPrivate
{
public:
	BlockMemoryAllocatorPrivate();
	
	void* alloc(uint32_t);
	void free(void*);
	
	bool validate(void*, bool abortOnFail = true);
	
	void flushUnusedBlocks();
	
	void printInfo();
	
private:
	CriticalSection _csLock;
	std::list<MemoryChunk> _chunks;
	
	SmallMemoryBlockAllocator<smallBlockSize> _allocatorSmall;
	SmallMemoryBlockAllocator<mediumBlockSize> _allocatorMedium;
};

BlockMemoryAllocator::BlockMemoryAllocator()
{
	ET_PIMPL_INIT(BlockMemoryAllocator)
}

BlockMemoryAllocator::~BlockMemoryAllocator()
{
	ET_PIMPL_FINALIZE(BlockMemoryAllocator)
}

void* BlockMemoryAllocator::allocate(size_t sz)
	{ return _private->alloc(alignUpTo(sz & 0xffffffff, minimumAllocationSize)); }

void BlockMemoryAllocator::release(void* ptr)
	{ _private->free(ptr); }

bool BlockMemoryAllocator::validatePointer(void* ptr, bool abortOnFail)
	{ return _private->validate(ptr, abortOnFail); }

void BlockMemoryAllocator::printInfo() const
	{ _private->printInfo(); }

void BlockMemoryAllocator::flushUnusedBlocks()
	{ _private->flushUnusedBlocks(); }

/*
 * Private
 */

BlockMemoryAllocatorPrivate::BlockMemoryAllocatorPrivate()
{
	_chunks.emplace_back(defaultChunkSize);
}

void* BlockMemoryAllocatorPrivate::alloc(uint32_t allocSize)
{
	CriticalSectionScope lock(_csLock);
	
	void* result = nullptr;
	
	if ((allocSize <= smallBlockSize) && _allocatorSmall.haveFreeBlocks() && _allocatorSmall.allocate(result))
		return result;
	
	if ((allocSize <= mediumBlockSize) && _allocatorMedium.haveFreeBlocks() && _allocatorMedium.allocate(result))
		return result;
	
	for (MemoryChunk& chunk : _chunks)
	{
		if (chunk.allocate(allocSize, result))
			return result;
	}
	
	_chunks.emplace_back(alignUpTo(allocSize, defaultChunkSize));
	
	auto& lastChunk = _chunks.back();
	lastChunk.allocate(allocSize, result);
	
	return result;
}

bool BlockMemoryAllocatorPrivate::validate(void* ptr, bool abortOnFail)
{
	if (ptr == nullptr)
		return true;
	
	CriticalSectionScope lock(_csLock);
	
	if (_allocatorSmall.containsPointer(ptr))
		return true;
	
	if (_allocatorMedium.containsPointer(ptr))
		return true;

	auto charPtr = static_cast<char*>(ptr);
	for (MemoryChunk& chunk : _chunks)
	{
		if (chunk.containsPointer(charPtr))
			return true;
	}
	
	if (abortOnFail)
	{
		uint64_t address = reinterpret_cast<uint64_t>(ptr);
		ET_FAIL_FMT("Pointer being freed (0x%016llx) was not allocated via this allocator.", address);
	}
	
	return false;
}

void BlockMemoryAllocatorPrivate::flushUnusedBlocks()
{
	CriticalSectionScope lock(_csLock);
	
	uint32_t blocksFlushed = 0;
	uint32_t memoryReleased = 0;
	
	auto i = _chunks.begin();
	while (i != _chunks.end())
	{
		bool shouldRemove = true;
		auto first = i->firstInfo;
		while (first != i->lastInfo)
		{
			if (first->allocated == allocatedValue)
			{
				shouldRemove = false;
				break;
			}
			++first;
		}
		
		if (shouldRemove)
		{
			memoryReleased += i->size;
			i = _chunks.erase(i);
			++blocksFlushed;
		}
		else
		{
			++i;
		}
	}
	
	if (blocksFlushed > 0)
	{
		log::info("[BlockMemoryAllocator] %u blocks flushed, total memory released: %u", blocksFlushed, memoryReleased);
	}
}

void BlockMemoryAllocatorPrivate::free(void* ptr)
{
	if (ptr == nullptr) return;
	
	CriticalSectionScope lock(_csLock);
	
	if (_allocatorSmall.containsPointer(ptr))
	{
		_allocatorSmall.free(ptr);
	}
	else if (_allocatorMedium.containsPointer(ptr))
	{
		_allocatorMedium.free(ptr);
	}
	else
	{
		auto charPtr = static_cast<char*>(ptr);
		for (MemoryChunk& chunk : _chunks)
		{
			if (chunk.free(charPtr))
				return;
		}
		
		ET_FAIL_FMT("Pointer being freed (0x%016llx) was not allocated via this allocator.", (int64_t)ptr);
	}
}

void BlockMemoryAllocatorPrivate::printInfo()
{
	log::info("Memory allocator has %zu chunks:", _chunks.size());
	log::info("{");
	for (auto& chunk : _chunks)
	{
		log::info("\t{");
#	if (ET_DEBUG)
		for (uint32_t i = 0; i < maximumAllocationStatisticsSize - 1; ++i)
		{
			if (chunk.allocationStatistics[i] > 0)
			{
				log::info("\t\t%04u-%04u : live: %u (alloc: %u, freed: %u)", i * minimumAllocationStatisticsSize,
					(i+1) * minimumAllocationStatisticsSize, chunk.allocationStatistics[i] - chunk.deallocationStatistics[i],
					chunk.allocationStatistics[i], chunk.deallocationStatistics[i]);
			}
		}
		
		auto j = (maximumAllocationStatisticsSize - 1);
		if (chunk.allocationStatistics[j] > 0)
		{
			log::info("\t\t%04u-.... : live: %u (alloc: %u, freed: %u)", j * minimumAllocationStatisticsSize,
				chunk.allocationStatistics[j] - chunk.deallocationStatistics[j],
				chunk.allocationStatistics[j], chunk.deallocationStatistics[j]);
		}
		log::info("\t\t------------");
#	endif
		
		uint32_t allocatedMemory = 0;
		auto i = chunk.firstInfo;
		while (i < chunk.lastInfo)
		{
			allocatedMemory += (i->length & (~i->allocated));
			++i;
		}
		log::info("\t\tTotal memory used: %u (%uKb, %uMb) of %u (%uKb, %uMb)", allocatedMemory, allocatedMemory / 1024,
			allocatedMemory / megabytes, chunk.size, chunk.size / 1024, chunk.size / megabytes);
		log::info("\t}");
	}
	
	uint32_t allocatedBlocks = 0;

	log::info("\t0...48 bytes");
	log::info("\t{");
	allocatedBlocks = 0;
	for (auto i = _allocatorSmall.firstBlock; i != _allocatorSmall.lastBlock; ++i)
		allocatedBlocks += i->blockAllocated;
	log::info("\t\tallocated blocks : %u of %lld", allocatedBlocks, (int64_t)(_allocatorSmall.lastBlock - _allocatorSmall.firstBlock));
	log::info("\t\tcurrent offset : %lld", (int64_t)(_allocatorSmall.currentBlock - _allocatorSmall.firstBlock));
	log::info("\t},");
	
	log::info("\t48...96");
	log::info("\t{");
	allocatedBlocks = 0;
	for (auto i = _allocatorMedium.firstBlock; i != _allocatorMedium.lastBlock; ++i)
		allocatedBlocks += i->blockAllocated;
	log::info("\t\tallocated blocks : %u of %lld", allocatedBlocks,  (int64_t)(_allocatorMedium.lastBlock - _allocatorMedium.firstBlock));
	log::info("\t\tcurrent offset : %lld", (int64_t)(_allocatorMedium.currentBlock - _allocatorMedium.firstBlock));
	log::info("\t}");
	
	log::info("}");
}

/*
 * Chunk
 */
MemoryChunk::MemoryChunk(uint32_t capacity) :
	size(capacity)
{
	static std::atomic<uint32_t> counter(0);
	index = counter++;
	
#if (ET_DEBUG)
	allocationStatistics.fill(0);
	deallocationStatistics.fill(0);
#endif
	
	uint32_t maxInfoChunks = capacity / minimumAllocationSize + 1;
	actualDataOffset = alignUpTo(maxInfoChunks * sizeof(MemoryChunkInfo), minimumAllocationSize);
	size_t sizeToAllocate = alignUpTo(actualDataOffset + capacity, minimumAllocationSize);
	
#if (ET_PLATFORM_APPLE)
	
	void* allocatedPtr = nullptr;
	posix_memalign(&allocatedPtr, minimumAllocationSize, sizeToAllocate);
	allocatedMemoryBegin = static_cast<char*>(allocatedPtr);
	
#elif (ET_PLATFORM_WIN)
	
	allocatedMemoryBegin = static_cast<char*>(_aligned_malloc(sizeToAllocate, minimumAllocationSize));
	
#else
#
#	error Use any available aligned malloc
#
#endif
	
	allocatedMemoryEnd = allocatedMemoryBegin + actualDataOffset + capacity;
	firstInfo = reinterpret_cast<MemoryChunkInfo*>(allocatedMemoryBegin);
	
	firstInfo->allocated = notAllocatedValue;
	firstInfo->begin = 0;
	firstInfo->length = capacity;
	
	lastInfo = firstInfo + 1;
}

MemoryChunk::~MemoryChunk()
{
	if (allocatedMemoryBegin)
	{
		log::ConsoleOutput lOut;

		std::vector<uint32_t> detectedLeaks;
		detectedLeaks.reserve(1024);

		uint32_t totalLeaked = 0;
		MemoryChunkInfo* info = firstInfo;
		while (info < lastInfo)
		{
			if (info->allocated == allocatedValue)
			{
				totalLeaked += info->length;
				detectedLeaks.emplace_back(info->allocIndex);
			}
			++info;
		}

		if (totalLeaked > 0)
		{
			char buffer[1024 * 10] = { };

			int printPos = 0;
			for (uint32_t i : detectedLeaks)
				printPos += sprintf(buffer + printPos, "%u, ", i);
			buffer[printPos - 2] = 0;

			lOut.info("Total memory leaked: %llu, use debug funtion:", totalLeaked);
			lOut.info("et::MemoryAllocatorBase::allocateOnBreaks({ %s })", buffer);
		}
		
#	if (ET_PLATFORM_WIN)
		_aligned_free(allocatedMemoryBegin);
#	else
		::free(allocatedMemoryBegin);
#	endif
	}
}

bool MemoryChunk::allocate(uint32_t sizeToAllocate, void*& result)
{
	MemoryChunkInfo* info = firstInfo;
	while (info < lastInfo)
	{
		validateInfo(info);
		if ((info->allocated & info->length) >= sizeToAllocate)
		{
			auto remaining = info->length - sizeToAllocate;
			
			info->allocated = allocatedValue;
			info->length = sizeToAllocate;
#		if ET_DEBUG
			info->allocIndex = MemoryAllocatorBase::allocationIndex++;
			if (_breakOnAllocations.count(info->allocIndex))
			{
				debug::debugBreak();
			}
#		endif
			
			if (remaining > minimumAllocationSize)
			{
				auto nextInfo = info + 1;
				
				if (nextInfo >= lastInfo) // last one reached
				{
					lastInfo = nextInfo + 1;
				}
				else
				{
					for (auto i = lastInfo, prev = lastInfo - 1; i > nextInfo; --i, --prev)
						prev->swapWith(i);
					
					++lastInfo;
				}
				
				nextInfo->allocated = notAllocatedValue;
				nextInfo->begin = info->begin + info->length;
				nextInfo->length = remaining;
			}
			
			result = allocatedMemoryBegin + actualDataOffset + info->begin;
			
#		if (ET_DEBUG)
			uint32_t allocIndex = std::min(maximumAllocationStatisticsSize - 1, sizeToAllocate / minimumAllocationStatisticsSize);
			++allocationStatistics[allocIndex];
			if (breakOnAllocation)
				log::info("Allocated %u bytes (%uKb, %uMb)", sizeToAllocate, sizeToAllocate / 1024, sizeToAllocate / megabytes);
#		endif
			
			return true;
		}
		
		++info;
	}
	
	return false;
}

inline void MemoryChunk::validateInfo(et::MemoryChunkInfo* info)
{
	ET_ASSERT((info->allocated == allocatedValue) || (info->allocated == notAllocatedValue));
	ET_ASSERT(info->begin < size);
	ET_ASSERT(info->length <= size);
	ET_ASSERT(info->begin + info->length <= size);
	(void)info;
}

bool MemoryChunk::containsPointer(char* ptr)
{
	if ((ptr < allocatedMemoryBegin + actualDataOffset) || (ptr >= allocatedMemoryEnd)) return false;
	uint32_t offset = static_cast<uint32_t>(ptr - (allocatedMemoryBegin + actualDataOffset));
	
	auto i = firstInfo;
	while (i < lastInfo)
	{
		if (i->begin == offset)
			return true;
		++i;
	}
	
	return false;
}

bool MemoryChunk::free(char* ptr)
{
	if ((ptr < allocatedMemoryBegin + actualDataOffset) || (ptr >= allocatedMemoryEnd)) return false;
	uint32_t offset = static_cast<uint32_t>(ptr - (allocatedMemoryBegin + actualDataOffset));
	
	auto i = firstInfo;
	while (i < lastInfo)
	{
		validateInfo(i);
		if (i->begin == offset)
		{
			if (i->allocated == notAllocatedValue)
			{
				ET_FAIL_FMT("Pointer being freed (0x%016llx) was already deleted from this memory chunk.", reinterpret_cast<uint64_t>(ptr));
				return false;
			}
			else
			{
#			if ET_DEBUG
				uint32_t deallocIndex = std::min(maximumAllocationStatisticsSize - 1, i->length / minimumAllocationStatisticsSize);
				++deallocationStatistics[deallocIndex];
				if (breakOnAllocation)
					log::info("Deallocated %u bytes (%uKb, %uMb)", i->length, i->length / 1024, i->length / megabytes);
#			endif
				
				i->allocated = notAllocatedValue;
				compress();
				return true;
			}
		}
		++i;
	}
	
	return false;
}

void MemoryChunk::compress()
{
	auto i = firstInfo;
	
	while (i < lastInfo)
	{
		validateInfo(i);
		if (i->allocated == notAllocatedValue)
		{
			auto nextInfo = i + 1;
			if ((nextInfo < lastInfo) && (nextInfo->allocated == notAllocatedValue))
			{
				i->length += nextInfo->length;
				
				auto next = nextInfo + 1;
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

}
