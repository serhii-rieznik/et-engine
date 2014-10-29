/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/et.h>
#include <et/threading/criticalsection.h>

#define ET_ENABLE_SMALL_MEMORY_BLOCKS	1

namespace et
{
	enum : uint32_t
	{
		megabytes = 1024 * 1024,
		allocatedValue = 0x00000000,
		notAllocatedValue = 0xffffffff,
		defaultChunkSize = 16 * megabytes,
		minimumAllocationSize = 64,
		maximumAllocationStatisticsSize = (2048 + minimumAllocationSize) / minimumAllocationSize,
		smallMemoryBlockSize = 64,
		smallMemoryBlocksCount = 8 * megabytes / (smallMemoryBlockSize + 4),
	};

	struct MemoryChunkInfo
	{
		uint32_t allocated = notAllocatedValue;
		uint32_t begin = 0;
		uint32_t length = 0;
	};
	
	class MemoryChunk
	{
	public:
		MemoryChunk(uint32_t);
		
		~MemoryChunk();
		
		MemoryChunk(MemoryChunk&& m)
		{
			size = m.size;
			actualDataOffset = m.actualDataOffset;
			firstInfo = m.firstInfo;
			lastInfo = m.lastInfo;
			allocatedMemoryBegin = m.allocatedMemoryBegin;
			allocatedMemoryEnd = m.allocatedMemoryEnd;
			
			m.size = 0;
			m.actualDataOffset = 0;
			m.firstInfo = nullptr;
			m.lastInfo = nullptr;
			m.allocatedMemoryBegin = nullptr;
			m.allocatedMemoryEnd = nullptr;
		}
		
		bool allocate(uint32_t size, void*& result);
		bool free(char*);
		
		bool containsPointer(char*);
		
		void compress();
		
#	if (ET_DEBUG)
		void setBreakOnAllocation()
			{ breakOnAllocation = true; }
#	endif
		
	private:
		void init(uint32_t);
		
	private:
		MemoryChunk(const MemoryChunk&) = delete;
		MemoryChunk& operator = (const BlockMemoryAllocator&) = delete;
		
	public:
		uint32_t compressCounter = 0;
		uint32_t size = 0;
		uint32_t actualDataOffset = 0;
		
		MemoryChunkInfo* firstInfo = nullptr;
		MemoryChunkInfo* lastInfo = nullptr;
		
		char* allocatedMemoryBegin = nullptr;
		char* allocatedMemoryEnd = nullptr;
		
#if (ET_DEBUG)
		uint32_t allocationStatistics[maximumAllocationStatisticsSize];
		uint32_t deallocationStatistics[maximumAllocationStatisticsSize];
		bool breakOnAllocation = false;
#endif
	};
	
	struct SmallMemoryBlock
	{
		uint32_t allocated = 0;
		char data[smallMemoryBlockSize];
	};
	
	class BlockMemorySmallBlockAllocator
	{
	public:
		BlockMemorySmallBlockAllocator();
		~BlockMemorySmallBlockAllocator();
		
		void* allocate();
		void free(void*);
		
		bool containsPointer(void*);
		
		SmallMemoryBlock* advance(SmallMemoryBlock*);
		
	public:
		SmallMemoryBlock* blocks = nullptr;
		SmallMemoryBlock* firstBlock = nullptr;
		SmallMemoryBlock* lastBlock = nullptr;
		SmallMemoryBlock* currentBlock = nullptr;
	};
	
	class BlockMemoryAllocatorPrivate
	{
	public:
		BlockMemoryAllocatorPrivate();
		
		void* alloc(uint32_t);
		void free(void*);
		
		bool validate(void*, bool abortOnFail = true);
		
		void printInfo();
		
	private:
		CriticalSection _csLock;
		std::list<MemoryChunk> _chunks;
		
#if ET_ENABLE_SMALL_MEMORY_BLOCKS
		BlockMemorySmallBlockAllocator _smallBlockAllocator;
#endif
	};
}

using namespace et;

inline uint32_t alignUpTo(uint32_t sz, uint32_t al)
{
	ET_ASSERT(sz > 0)
	auto m = al-1;
	return sz + m & (~m);
}

inline uint32_t alignDownTo(uint32_t sz, uint32_t al)
{
	ET_ASSERT(sz > 0)
	return sz & (~(al-1));
}

BlockMemoryAllocator::BlockMemoryAllocator()
{
	ET_PIMPL_INIT(BlockMemoryAllocator)
}

BlockMemoryAllocator::~BlockMemoryAllocator()
{
	ET_PIMPL_FINALIZE(BlockMemoryAllocator)
}

void* BlockMemoryAllocator::alloc(size_t sz)
	{ return _private->alloc(alignUpTo(sz & 0xffffffff, minimumAllocationSize)); }

void BlockMemoryAllocator::free(void* ptr)
	{ _private->free(ptr); }

bool BlockMemoryAllocator::validatePointer(void* ptr, bool abortOnFail)
	{ return _private->validate(ptr, abortOnFail); }

void BlockMemoryAllocator::printInfo() const
	{ _private->printInfo(); }

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
#if ET_ENABLE_SMALL_MEMORY_BLOCKS
	if (allocSize <= smallMemoryBlockSize)
	{
		result = _smallBlockAllocator.allocate();
	}
	else
#endif
	{
		for (MemoryChunk& chunk : _chunks)
		{
			if (chunk.allocate(allocSize, result))
				return result;
		}
		
		_chunks.emplace_back(alignUpTo(allocSize, defaultChunkSize));
		_chunks.back().allocate(allocSize, result);
	}
	return result;
}

bool BlockMemoryAllocatorPrivate::validate(void* ptr, bool abortOnFail)
{
	if (ptr == nullptr)
		return true;
	
	CriticalSectionScope lock(_csLock);
	
#if ET_ENABLE_SMALL_MEMORY_BLOCKS
	if (_smallBlockAllocator.containsPointer(ptr))
		return true;
#endif
	auto charPtr = static_cast<char*>(ptr);
	for (MemoryChunk& chunk : _chunks)
	{
		if (chunk.containsPointer(charPtr))
			return true;
	}
	
	if (abortOnFail)
	{
		ET_FAIL_FMT("Pointer being freed (0x%016llx) was not allocated via this allocator.", (int64_t)ptr);
	}
	
	return false;
}

void BlockMemoryAllocatorPrivate::free(void* ptr)
{
	if (ptr == nullptr) return;
	
	CriticalSectionScope lock(_csLock);
	
#if ET_ENABLE_SMALL_MEMORY_BLOCKS
	if (_smallBlockAllocator.containsPointer(ptr))
	{
		_smallBlockAllocator.free(ptr);
	}
	else
#endif
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
	
#if (ET_DEBUG)
		log::info("{");
		for (auto& chunk : _chunks)
		{
			log::info("\t{");
			for (uint32_t i = 0; i < maximumAllocationStatisticsSize - 1; ++i)
			{
				if (chunk.allocationStatistics[i] > 0)
				{
					log::info("\t\t%04u-%04u : live: %u (alloc: %u, freed: %u)", i * minimumAllocationSize,
						(i+1) * minimumAllocationSize, chunk.allocationStatistics[i] - chunk.deallocationStatistics[i],
						chunk.allocationStatistics[i], chunk.deallocationStatistics[i]);
				}
			}
			
			auto j = (maximumAllocationStatisticsSize - 1);
			if (chunk.allocationStatistics[j] > 0)
			{
				log::info("\t\t%04u-.... : live: %u (alloc: %u, freed: %u)", j * minimumAllocationSize,
					chunk.allocationStatistics[j] - chunk.deallocationStatistics[j],
					chunk.allocationStatistics[j], chunk.deallocationStatistics[j]);
			}
			
			uint32_t allocatedMemory = 0;
			auto i = chunk.firstInfo;
			while (i < chunk.lastInfo)
			{
				allocatedMemory += (i->length & (~i->allocated));
				++i;
			}
			log::info("\t\t------------");
			log::info("\t\tTotal memory used: %u (%uKb, %uMb) of %u (%uKb, %uMb)", allocatedMemory, allocatedMemory / 1024,
				allocatedMemory / megabytes, chunk.size, chunk.size / 1024, chunk.size / megabytes);
			log::info("\t}");
		}
		
#	if ET_ENABLE_SMALL_MEMORY_BLOCKS
		uint32_t allocatedBlocks = 0;
		for (auto i = _smallBlockAllocator.firstBlock; i != _smallBlockAllocator.lastBlock; ++i)
			allocatedBlocks += i->allocated;

		log::info("\tsmall memory block allocator:");
		log::info("\t{");
		log::info("\t\tallocated blocks : %u", allocatedBlocks);
		log::info("\t\tcurrent offset : %lld of %lld", (int64_t)(_smallBlockAllocator.currentBlock - _smallBlockAllocator.firstBlock),
			(int64_t)(_smallBlockAllocator.lastBlock - _smallBlockAllocator.firstBlock));
		log::info("\t}");
#	endif
	
#endif
	log::info("}");
}

/*
 * Chunk
 */
MemoryChunk::MemoryChunk(uint32_t capacity)
{
#if (ET_DEBUG)
	memset(allocationStatistics, 0, sizeof(allocationStatistics));
	memset(deallocationStatistics, 0, sizeof(deallocationStatistics));
#endif
	
	size = capacity;
	
	auto maxPossibleInfoChunks = (capacity / minimumAllocationSize);
	
	actualDataOffset = (maxPossibleInfoChunks + 1) * sizeof(MemoryChunkInfo);
	allocatedMemoryBegin = static_cast<char*>(malloc(actualDataOffset + capacity));
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
		free(allocatedMemoryBegin);
}

bool MemoryChunk::allocate(uint32_t sizeToAllocate, void*& result)
{
	MemoryChunkInfo* info = firstInfo;
	while (info < lastInfo)
	{
		if ((info->allocated & info->length) >= sizeToAllocate)
		{
			auto remaining = info->length - sizeToAllocate;
			
			info->allocated = allocatedValue;
			info->length = sizeToAllocate;
			
			if (remaining > minimumAllocationSize)
			{
				auto nextInfo = info + 1;
				if (nextInfo >= lastInfo) // last one reached
				{
					lastInfo = nextInfo + 1;
				}
				else
				{
					for (auto i = lastInfo; i > nextInfo; --i)
						std::swap(*(i-1), *i);
					++lastInfo;
				}
				
				nextInfo->allocated = notAllocatedValue;
				nextInfo->begin = info->begin + info->length;
				nextInfo->length = remaining;
			}
			
			result = allocatedMemoryBegin + actualDataOffset + info->begin;
			
#		if (ET_DEBUG)
			uint32_t allocIndex = etMin(uint32_t(maximumAllocationStatisticsSize), sizeToAllocate / minimumAllocationSize) - 1;
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
		if (i->begin == offset)
		{
			i->allocated = notAllocatedValue;
			
#		if ET_DEBUG
			uint32_t deallocIndex = etMin(uint32_t(maximumAllocationStatisticsSize), i->length / minimumAllocationSize) - 1;
			++deallocationStatistics[deallocIndex];
			if (breakOnAllocation)
				log::info("Deallocated %u bytes (%uKb, %uMb)", i->length, i->length / 1024, i->length / megabytes);
#		endif
			
			compress();
			return true;
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
		if (i->allocated == notAllocatedValue)
		{
			auto nextInfo = i + 1;
			if ((nextInfo < lastInfo) && (nextInfo->allocated == notAllocatedValue))
			{
				i->length += nextInfo->length;
				for (auto j = nextInfo; j < lastInfo; ++j)
					std::swap(*(j+1), *j);
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

BlockMemorySmallBlockAllocator::BlockMemorySmallBlockAllocator()
{
	auto sizeToAllocate = smallMemoryBlocksCount * sizeof(SmallMemoryBlock);
	blocks = reinterpret_cast<SmallMemoryBlock*>(calloc(1, sizeToAllocate));
	firstBlock = blocks;
	currentBlock = firstBlock;
	
	lastBlock = firstBlock + smallMemoryBlocksCount;
}

BlockMemorySmallBlockAllocator::~BlockMemorySmallBlockAllocator()
{
	free(blocks);
}

void* BlockMemorySmallBlockAllocator::allocate()
{
	auto startBlock = currentBlock;
	
	while (currentBlock->allocated)
	{
		currentBlock = advance(currentBlock);
		if (currentBlock == startBlock)
			ET_FAIL("Failed to allocate small memory block.");
	}
	
	currentBlock->allocated = true;
	void* result = currentBlock->data;
	currentBlock = advance(currentBlock);
	return result;
}

bool BlockMemorySmallBlockAllocator::containsPointer(void* ptr)
{
	SmallMemoryBlock* block = reinterpret_cast<SmallMemoryBlock*>(reinterpret_cast<char*>(ptr) - 4);
	return (block >= firstBlock) && (block < lastBlock);
}

void BlockMemorySmallBlockAllocator::free(void* ptr)
{
	SmallMemoryBlock* block = reinterpret_cast<SmallMemoryBlock*>(reinterpret_cast<char*>(ptr) - 4);
	
	if ((ptr < firstBlock) || (ptr > lastBlock))
		ET_FAIL_FMT("Pointer being freed (0x%016llx) was not allocated via this allocator.", (int64_t)ptr);
	
	block->allocated = 0;
}

SmallMemoryBlock* BlockMemorySmallBlockAllocator::advance(SmallMemoryBlock* b)
{
	++b;
	return (b == lastBlock) ? firstBlock : b;
}
