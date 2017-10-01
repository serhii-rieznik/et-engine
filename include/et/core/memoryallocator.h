/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */


#pragma once

#if !defined(ET_CORE_INCLUDES)
#	error "Do not include this file directly, it is automatically included in et.h"
#endif

namespace et
{
class BlockMemoryAllocator;
class ObjectFactory
{
public:
	ObjectFactory() = default;
		
	void setAllocator(BlockMemoryAllocator* al);
	BlockMemoryAllocator* allocator();
	const BlockMemoryAllocator* allocator() const;
	
	template <typename O, typename ... args>
	O* createObject(args&&...a);
	
	template <typename O>
	void deleteObject(O* obj);
	
private:
	ET_DENY_COPY(ObjectFactory);
	BlockMemoryAllocator* _allocator = nullptr;
};

class BlockMemoryAllocatorPrivate;
class BlockMemoryAllocator
{
public:
#if (ET_DEBUG)
	static void allocateOnBreaks(const std::set<uint64_t>&);
	static std::atomic<uint64_t> allocationIndex;
#endif

public:
	BlockMemoryAllocator();
	~BlockMemoryAllocator();
	
	void* allocate(uint64_t);
	bool validatePointer(void*, bool = true);
	void release(void* ptr);

	void printInfo() const;
	void flushUnusedBlocks();
			
private:
	ET_DECLARE_PIMPL(BlockMemoryAllocator, 256);
};

/*
 * ObjectFactory implementation
 */
inline void ObjectFactory::setAllocator(BlockMemoryAllocator* al)
{
	_allocator = al;
}

inline BlockMemoryAllocator* ObjectFactory::allocator()
{
	return _allocator;
}

inline const BlockMemoryAllocator* ObjectFactory::allocator() const
{
	return _allocator;
}

template <typename O, typename ... args>
inline O* ObjectFactory::createObject(args&&...a)
{
	ET_ASSERT(_allocator != nullptr);
	return new (_allocator->allocate(sizeof(O))) O(std::forward<args>(a)...); \
}

template <typename O>
inline void ObjectFactory::deleteObject(O* obj)
{
	ET_ASSERT(_allocator != nullptr);
	if (obj != nullptr)
	{
#	if (ET_DEBUG)
		_allocator->validatePointer(obj);
#	endif
		obj->~O();
		_allocator->release(obj);
	}
}

}
