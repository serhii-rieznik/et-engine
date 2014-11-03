/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/et.h>
#include <et/threading/criticalsection.h>

namespace et
{
	enum : uint32_t
	{
		megabytes = 1024 * 1024,
		allocatedValue = 0x00000000,
		notAllocatedValue = 0xffffffff,
		defaultChunkSize = 16 * megabytes,
		minimumAllocationSize = 16,
		minimumAllocationStatisticsSize = 96,
		maximumAllocationStatisticsSize = (uint32_t)((2048 + minimumAllocationStatisticsSize) / minimumAllocationStatisticsSize)
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
		StaticDataStorage<uint32_t, maximumAllocationStatisticsSize> allocationStatistics;
		StaticDataStorage<uint32_t, maximumAllocationStatisticsSize> deallocationStatistics;
		bool breakOnAllocation = false;
#endif
	};
	
	
	template <int blockSize>
	class BlockMemorySmallBlockAllocator
	{
	public:
		BlockMemorySmallBlockAllocator()
		{
			auto sizeToAllocate = blocksCount * sizeof(SmallMemoryBlock);
			blocks = reinterpret_cast<SmallMemoryBlock*>(calloc(1, sizeToAllocate));
			firstBlock = blocks;
			currentBlock = firstBlock;
			lastBlock = firstBlock + blocksCount;
		}
		
		~BlockMemorySmallBlockAllocator()
		{
			log::ConsoleOutput lOut;
			for (auto i = firstBlock; i != lastBlock; ++i)
			{
				if (i->allocated)
					lOut.info("Memory leak detected: %d bytes, offset %lld", blockSize, (int64_t)(i - firstBlock));
			}
			::free(blocks);
		}
		
		bool allocate(void*& result)
		{
			auto startBlock = currentBlock;
			
			while (currentBlock->allocated)
			{
				currentBlock = advance(currentBlock);
				if (currentBlock == startBlock)
				{
					_haveFreeBlocks = false;
					log::warning("Small memory block (%d) filled.", blockSize);
					return false;
				}
			}
			
			currentBlock->allocated = 1;
			
			result = currentBlock->data;
			currentBlock = advance(currentBlock);
			
			return true;
		}
		
		void free(void* ptr)
		{
			SmallMemoryBlock* block = reinterpret_cast<SmallMemoryBlock*>(reinterpret_cast<char*>(ptr) - 4);
			
			if ((ptr < firstBlock) || (ptr > lastBlock))
				ET_FAIL_FMT("Pointer being freed (0x%016llx) was not allocated via this allocator.", (int64_t)ptr);
			
			block->allocated = 0;
			
			if (_haveFreeBlocks == false)
				currentBlock = block;

			_haveFreeBlocks = true;
		}
		
		bool containsPointer(void* ptr)
		{
			SmallMemoryBlock* block = reinterpret_cast<SmallMemoryBlock*>(reinterpret_cast<char*>(ptr) - 4);
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
			uint32_t allocated = 0;
			char data[blockSize];
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
		
		void printInfo();
		
	private:
		CriticalSection _csLock;
		std::list<MemoryChunk> _chunks;
		
		BlockMemorySmallBlockAllocator<48> _allocator48;
		BlockMemorySmallBlockAllocator<96> _allocator96;
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
	
	auto allocScale = (allocSize - 1) / 48;
	switch (allocScale)
	{
		case 0 :
		{
			if (_allocator48.haveFreeBlocks() && _allocator48.allocate(result))
				return result;
		}
			
		case 1 :
		{
			if (_allocator96.haveFreeBlocks() && _allocator96.allocate(result))
				return result;
		}
			
		default:
		{
			for (MemoryChunk& chunk : _chunks)
			{
				if (chunk.allocate(allocSize, result))
					return result;
			}
			
			_chunks.emplace_back(alignUpTo(allocSize, defaultChunkSize));
			_chunks.back().allocate(allocSize, result);
		}
	}
	
	return result;
}

bool BlockMemoryAllocatorPrivate::validate(void* ptr, bool abortOnFail)
{
	if (ptr == nullptr)
		return true;
	
	CriticalSectionScope lock(_csLock);
	
	if (_allocator48.containsPointer(ptr))
		return true;
	
	if (_allocator96.containsPointer(ptr))
		return true;

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
	
	if (_allocator48.containsPointer(ptr))
	{
		_allocator48.free(ptr);
	}
	else if (_allocator96.containsPointer(ptr))
	{
		_allocator96.free(ptr);
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

	log::info("\t0...48 bytes {");
	allocatedBlocks = 0;
	for (auto i = _allocator48.firstBlock; i != _allocator48.lastBlock; ++i) allocatedBlocks += i->allocated;
	log::info("\t\tallocated blocks : %u of %lld", allocatedBlocks, (int64_t)(_allocator48.lastBlock - _allocator48.firstBlock));
	log::info("\t\tcurrent offset : %lld", (int64_t)(_allocator48.currentBlock - _allocator48.firstBlock));
	log::info("\t},");
	
	log::info("\t48...96 {");
	allocatedBlocks = 0;
	for (auto i = _allocator96.firstBlock; i != _allocator96.lastBlock; ++i) allocatedBlocks += i->allocated;
	log::info("\t\tallocated blocks : %u of %lld", allocatedBlocks,  (int64_t)(_allocator96.lastBlock - _allocator96.firstBlock));
	log::info("\t\tcurrent offset : %lld", (int64_t)(_allocator96.currentBlock - _allocator96.firstBlock));
	log::info("\t}");
	
	log::info("}");
}

/*
 * Chunk
 */
MemoryChunk::MemoryChunk(uint32_t capacity)
{
#if (ET_DEBUG)
	allocationStatistics.fill(0);
	deallocationStatistics.fill(0);
#endif
	
	size = capacity;
	
	actualDataOffset = (capacity / minimumAllocationSize + 1) * sizeof(MemoryChunkInfo);
	allocatedMemoryBegin = static_cast<char*>(malloc(actualDataOffset + capacity));
	allocatedMemoryEnd = allocatedMemoryBegin + actualDataOffset + capacity;
	firstInfo = reinterpret_cast<MemoryChunkInfo*>(allocatedMemoryBegin);
	
	firstInfo->allocated = notAllocatedValue;
	firstInfo->begin = 0;
	firstInfo->length = capacity;
	
	lastInfo = firstInfo + 1;
}

template <typename ...args>
void printlog(const char* fmt, args&... a)
{
	printf(fmt, a...);
}

MemoryChunk::~MemoryChunk()
{
	if (allocatedMemoryBegin)
	{
		log::ConsoleOutput lOut;
		
		MemoryChunkInfo* info = firstInfo;
		while (info < lastInfo)
		{
			if (info->allocated == allocatedValue)
				lOut.info("Memory leak detected: %u bytes\n", info->length);
			
			++info;
		}
		
		::free(allocatedMemoryBegin);
	}
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
			uint32_t allocIndex = etMin(maximumAllocationStatisticsSize - 1, sizeToAllocate / minimumAllocationStatisticsSize);
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
			uint32_t deallocIndex = etMin(maximumAllocationStatisticsSize - 1, i->length / minimumAllocationStatisticsSize);
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
