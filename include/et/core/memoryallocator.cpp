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
std::atomic<uint32_t> BlockMemoryAllocator::allocationIndex(0);
static std::set<uint32_t> _breakOnAllocations;
void BlockMemoryAllocator::allocateOnBreaks(const std::set<uint32_t>& indices)
{
	_breakOnAllocations.insert(indices.begin(), indices.end());
}
#endif

enum : uint32_t
{
	megabytes = 1024 * 1024,
	allocatedValue = 0x00000000,
	notAllocatedValue = 0xffffffff,
	defaultChunkSize = 16 * megabytes,
	allocGranularity = 4 * megabytes,
	minimumAllocationSize = 128,
	smallBlockSize = 156,
	mediumBlockSize = 284,
};

class MemoryChunk
{
public:
	MemoryChunk(uint32_t);

	~MemoryChunk();

	bool allocate(uint32_t size, void*& result);
	bool containsPointer(char*);
	bool free(char*);

private:
	MemoryChunk(const MemoryChunk&) = delete;
	MemoryChunk& operator = (const BlockMemoryAllocator&) = delete;

public:
	RemoteHeap heap;
	char* allocatedMemoryBegin = nullptr;
	char* allocatedMemoryEnd = nullptr;
	char* actualDataMemory = nullptr;
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
		size_t sizeToAllocate = blocksCount * sizeof(SmallMemoryBlock);
	
	#if (ET_PLATFORM_WIN)
		 blocks = reinterpret_cast<SmallMemoryBlock*>(_aligned_malloc(sizeToAllocate, 32));
	#else
		#error Implement allocation
	#endif
		
		memset(blocks, 0, sizeToAllocate);

		firstBlock = blocks;
		currentBlock = firstBlock;
		lastBlock = firstBlock + blocksCount;
	}

	~SmallMemoryBlockAllocator()
	{
#if (ET_DEBUG)
		std::set<uint32_t> detectedLeaks;
		uint32_t totalLeaked = 0;

		log::ConsoleOutput lOut;
		for (auto i = firstBlock; i != lastBlock; ++i)
		{
			if (i->blockAllocated)
			{
				totalLeaked += blockSize;
				detectedLeaks.insert(i->allocIndex);
			}
		}

		if (totalLeaked > 0)
		{
			char buffer[1024 * 10] = {};

			int printPos = 0;
			for (uint32_t i : detectedLeaks)
			{
				printPos += sprintf(buffer + printPos, "%u, ", i);
				if ((printPos + 128) > sizeof(buffer))
					break;
			}
			buffer[printPos - 2] = 0;

			lOut.info("Total memory leaked: %llu from small blocks, use debug funtion:\n"
				"et::BlockMemoryAllocator::allocateOnBreaks({ %s })", totalLeaked, buffer);
		}
#endif
	#if (ET_PLATFORM_WIN)
		_aligned_free(blocks);
	#else
		#error Implement deallocation
	#endif
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
				if (!_warningShown)
				{
					log::warning("Small memory block (%d) filled.", blockSize);
					_warningShown = true;
				}
				return false;
			}
		}

		currentBlock->blockAllocated = 1;
#	if (ET_DEBUG)
		currentBlock->allocIndex = BlockMemoryAllocator::allocationIndex++;
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
	{
		return _haveFreeBlocks;
	}

private:
	enum
	{
		blocksCount = 8 * megabytes / (blockSize + 4)
	};

	struct SmallMemoryBlock
	{
		char data[blockSize];
		uint32_t blockAllocated : 1;
		uint32_t allocIndex : 31;
	};

	static_assert(sizeof(SmallMemoryBlock) % 32 == 0,
		"Invalid block size will cause troubles allocating aligned objects");

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
	bool _warningShown = false;
};

class BlockMemoryAllocatorPrivate
{
public:
	BlockMemoryAllocatorPrivate();
	~BlockMemoryAllocatorPrivate();

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
	uint32_t _smallAllocations = 0;
	uint32_t _mediumAllocations = 0;
	uint32_t _largeAllocations = 0;
	uint32_t _minAllocSize = std::numeric_limits<uint32_t>::max();
	uint32_t _maxAllocSize = 0;
};

BlockMemoryAllocator::BlockMemoryAllocator()
{
	ET_PIMPL_INIT(BlockMemoryAllocator);
}

BlockMemoryAllocator::~BlockMemoryAllocator()
{
	ET_PIMPL_FINALIZE(BlockMemoryAllocator);
}

void* BlockMemoryAllocator::allocate(uint32_t sz)
{
	return _private->alloc(alignUpTo(sz, uint32_t(minimumAllocationSize)));
}

void BlockMemoryAllocator::release(void* ptr)
{
	_private->free(ptr);
}

bool BlockMemoryAllocator::validatePointer(void* ptr, bool abortOnFail)
{
	return _private->validate(ptr, abortOnFail);
}

void BlockMemoryAllocator::printInfo() const
{
	_private->printInfo();
}

void BlockMemoryAllocator::flushUnusedBlocks()
{
	_private->flushUnusedBlocks();
}

/*
 * Private
 */
BlockMemoryAllocatorPrivate::BlockMemoryAllocatorPrivate()
{
	_chunks.emplace_back(defaultChunkSize);
}

BlockMemoryAllocatorPrivate::~BlockMemoryAllocatorPrivate()
{
	log::ConsoleOutput out;
	out.info("Allocation statistics: %u / %u / %u, sizes: %u .. %u", 
		_smallAllocations, _mediumAllocations, _largeAllocations,
		_minAllocSize, _maxAllocSize);
}

void* BlockMemoryAllocatorPrivate::alloc(uint32_t allocSize)
{
	CriticalSectionScope lock(_csLock);

	void* result = nullptr;

	_minAllocSize = std::min(_minAllocSize, allocSize);
	_maxAllocSize = std::max(_maxAllocSize, allocSize);
	
	if ((allocSize <= smallBlockSize) && _allocatorSmall.haveFreeBlocks() && _allocatorSmall.allocate(result))
	{
		++_smallAllocations;
		return result;
	}
	
	if ((allocSize <= mediumBlockSize) && _allocatorMedium.haveFreeBlocks() && _allocatorMedium.allocate(result))
	{
		++_mediumAllocations;
		return result;
	}

	for (MemoryChunk& chunk : _chunks)
	{
		if (chunk.allocate(allocSize, result))
			return result;
	}

	_chunks.emplace_back(alignUpTo(std::max(allocSize, uint32_t(defaultChunkSize)), uint32_t(allocGranularity)));

	MemoryChunk& lastChunk = _chunks.back();
	if (lastChunk.allocate(allocSize, result))
	{
		++_largeAllocations;
		return result;
	}

	ET_FAIL_FMT("Failed to allocate %u bytes", allocSize);
	return nullptr;
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

	char* charPtr = static_cast<char*>(ptr);
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
		if (i->heap.empty())
		{
			memoryReleased += i->heap.capacity();
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
	if (ptr == nullptr) 
		return;

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
		char* charPtr = static_cast<char*>(ptr);
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
		uint32_t allocatedMemory = chunk.heap.allocatedSize();
		log::info("\t\tTotal memory used: %u (%uKb, %uMb) of %u (%uKb, %uMb)", allocatedMemory, allocatedMemory / 1024,
			allocatedMemory / megabytes, chunk.heap.capacity(), chunk.heap.capacity() / 1024, chunk.heap.capacity() / megabytes);
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
	log::info("\t\tallocated blocks : %u of %lld", allocatedBlocks, (int64_t)(_allocatorMedium.lastBlock - _allocatorMedium.firstBlock));
	log::info("\t\tcurrent offset : %lld", (int64_t)(_allocatorMedium.currentBlock - _allocatorMedium.firstBlock));
	log::info("\t}");

	log::info("}");
}

/*
 * Chunk
 */
MemoryChunk::MemoryChunk(uint32_t capacity) :
	heap(capacity, minimumAllocationSize)
{
	uint32_t actualDataOffset = heap.requiredInfoSize();
	uint32_t totalSize = alignUpTo(actualDataOffset + capacity, uint32_t(minimumAllocationSize));

#if (ET_PLATFORM_APPLE)

	void* allocatedPtr = nullptr;
	posix_memalign(&allocatedPtr, minimumAllocationSize, totalSize);
	allocatedMemoryBegin = static_cast<char*>(allocatedPtr);

#elif (ET_PLATFORM_WIN)

	allocatedMemoryBegin = static_cast<char*>(_aligned_malloc(totalSize, minimumAllocationSize));

#else
#
#	error Use any available aligned malloc
#
#endif

	if (allocatedMemoryBegin == nullptr)
	{
		ET_FAIL_FMT("Failed to allocate %u bytes (%u requested + %u info)", totalSize, capacity, actualDataOffset);
	}

	actualDataMemory = allocatedMemoryBegin + actualDataOffset;
	allocatedMemoryEnd = allocatedMemoryBegin + totalSize;
	heap.setInfoStorage(allocatedMemoryBegin);
}

MemoryChunk::~MemoryChunk()
{
#if (ET_DEBUG)
	uint32_t totalLeaked = heap.allocatedSize();
	if (totalLeaked > 0)
	{
		log::ConsoleOutput lOut;
		lOut.info("Total memory leaked: %llu", totalLeaked);
	}
#endif

#if (ET_PLATFORM_WIN)
	_aligned_free(allocatedMemoryBegin);
#else
	::free(allocatedMemoryBegin);
#endif
}

bool MemoryChunk::allocate(uint32_t sizeToAllocate, void*& result)
{
	uint32_t offset = 0;
	if (heap.allocate(sizeToAllocate, offset))
	{
		result = actualDataMemory + offset;
		return true;
	}
	return false;
}

bool MemoryChunk::containsPointer(char* ptr)
{
	if ((ptr < actualDataMemory) || (ptr >= allocatedMemoryEnd))
		return false;

	return heap.containsAllocationWithOffset(static_cast<uint32_t>(ptr - actualDataMemory));
}

bool MemoryChunk::free(char* ptr)
{
	if ((ptr < actualDataMemory) || (ptr >= allocatedMemoryEnd)) return false;
	return heap.release(static_cast<uint32_t>(ptr - actualDataMemory));
}

}
