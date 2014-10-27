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
	static DefaultMemoryAllocator defaultMemoryAllocator;
	static BlockMemoryAllocator blockMemoryAllocator;
	static ObjectFactory objectFactory(&blockMemoryAllocator);
	
	DefaultMemoryAllocator& sharedDefaultAllocator()
		{ return defaultMemoryAllocator; }
	
	BlockMemoryAllocator& sharedBlockAllocator()
		{ return blockMemoryAllocator; }
	
	ObjectFactory& sharedObjectFactory()
		{ return objectFactory; }
	
	enum : uint32_t
	{
		allocatedValue = 0x00000000,
		notAllocatedValue = 0xffffffff,
		defaultChunkSize = 32 * (1024*1024),
		minimumAllocationSize = 32,
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
			
			allocations = m.allocations;
			actualDataOffset = m.actualDataOffset;
			
			firstInfo = m.firstInfo;
			lastInfo = m.lastInfo;
			
			allocatedMemoryBegin = m.allocatedMemoryBegin;
			allocatedMemoryEnd = m.allocatedMemoryEnd;
			
			m.size = 0;
			m.allocations = 0;
			m.actualDataOffset = 0;
			m.firstInfo = nullptr;
			m.lastInfo = nullptr;
			m.allocatedMemoryBegin = nullptr;
			m.allocatedMemoryEnd = nullptr;
		}
		
		bool allocate(uint32_t size, void*& result);
		bool free(char*);
		
		void compress();
		
	private:
		void init(uint32_t);
		
	private:
		MemoryChunk(const MemoryChunk&) = delete;
		MemoryChunk& operator = (const BlockMemoryAllocator&) = delete;
		
		void compressFrom(char*);
		
	public:
		uint32_t size = 0;
		uint32_t allocations = 0;
		uint32_t actualDataOffset = 0;
		MemoryChunkInfo* firstInfo = nullptr;
		MemoryChunkInfo* lastInfo = nullptr;
		char* allocatedMemoryBegin = nullptr;
		char* allocatedMemoryEnd = nullptr;
	};
	
	class BlockMemoryAllocatorPrivate
	{
	public:
		BlockMemoryAllocatorPrivate();
		
		void* alloc(uint32_t);
		void free(void*);
		
		void printInfo();
		
	private:
		CriticalSection _csLock;
		std::list<MemoryChunk> _chunks;
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
	static_assert(sizeof(BlockMemoryAllocatorPrivate) <= PrivateEstimatedSize,
		"Insufficient storage for MemoryAllocatorPrivate");
	
	_private = new(_privateData) BlockMemoryAllocatorPrivate;
}

BlockMemoryAllocator::~BlockMemoryAllocator()
	{ _private->~BlockMemoryAllocatorPrivate(); }

void* BlockMemoryAllocator::alloc(size_t sz)
	{ return _private->alloc(alignUpTo(sz & 0xffffffff, minimumAllocationSize)); }

void BlockMemoryAllocator::free(void* ptr)
	{ _private->free(ptr); }

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
	for (MemoryChunk& chunk : _chunks)
	{
		if (chunk.allocate(allocSize, result))
			return result;
	}
	
	_chunks.emplace_back(alignUpTo(allocSize, defaultChunkSize));
	_chunks.back().allocate(allocSize, result);
	
	return result;
}

void BlockMemoryAllocatorPrivate::free(void* ptr)
{
	if (ptr == nullptr) return;
	
	CriticalSectionScope lock(_csLock);
	
	auto charPtr = static_cast<char*>(ptr);
	for (MemoryChunk& chunk : _chunks)
	{
		if (chunk.free(charPtr))
			return;
	}
	
	ET_FAIL_FMT("Pointer being freed (0x%016llx) was not allocated via this allocator.", (int64_t)ptr);
}

void BlockMemoryAllocatorPrivate::printInfo()
{
	log::info("Memory allocator has %zu chunks:", _chunks.size());
	log::info("{");
	for (auto& chunk : _chunks)
	{
		log::info("\t{");
		MemoryChunkInfo* i = chunk.firstInfo;
		MemoryChunkInfo* j = i;
		uint32_t totalSize = 0;
		bool consistent = true;
		while (i < chunk.lastInfo)
		{
			totalSize += i->length;
			
			if (i != j)
			{
				if (j->begin + j->length != i->begin)
					consistent = false;
			}
			
			log::info("\t\toffset: %-8u\tsize: %-8u\tallocated: %s", i->begin, i->length,
				(i->allocated == allocatedValue) ? "YES" : "NO");
			j = i;
			++i;
		}
		log::info("\t} -> consistent: %s, total size: %u, computed size: %u",
			consistent ? "YES" : "NO", chunk.size, totalSize);
		ET_ASSERT(consistent);
	}
	log::info("}");
}

/*
 * Chunk
 */
MemoryChunk::MemoryChunk(uint32_t capacity)
{
	size = capacity;
	
	auto maxPossibleInfoChunks = (capacity / minimumAllocationSize);
	
	actualDataOffset = (maxPossibleInfoChunks + 1) * sizeof(MemoryChunkInfo);
	allocatedMemoryBegin = static_cast<char*>(calloc(1, actualDataOffset + capacity));
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
			return true;
		}
		
		++info;
	}
	
	return false;
}

bool MemoryChunk::free(char* ptr)
{
	auto offset = ptr - (allocatedMemoryBegin + actualDataOffset);
	
	if ((offset < 0) || (offset > size)) return false;
	
	auto i = firstInfo;
	while (i < lastInfo)
	{
		if (i->begin == offset)
		{
			i->allocated = notAllocatedValue;
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

void MemoryChunk::compressFrom(char* ptr)
{
	
}
