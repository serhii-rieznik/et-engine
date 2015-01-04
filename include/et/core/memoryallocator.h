/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */


#pragma once

#if !defined(ET_CORE_INCLUDES)
#	error "Do not include this file directly, it is automatically included in et.h"
#endif

namespace et
{
	class MemoryAllocatorBase
	{
	public:
		MemoryAllocatorBase() { }
		virtual ~MemoryAllocatorBase() { }
		
		virtual void* allocate(size_t) = 0;
		virtual void release(void* ptr) = 0;
		
		virtual bool validatePointer(void*, bool abortOnFail = true) { return false; }
		
		virtual void printInfo() const { }
		
	private:
		ET_DENY_COPY(MemoryAllocatorBase)
	};
	
	class ObjectFactory
	{
	public:
		ObjectFactory()
			{ }
		
		ObjectFactory(MemoryAllocatorBase* al) :
			_allocator(al) { }
		
		void setAllocator(MemoryAllocatorBase* al)
			{ _allocator = al; }
		
		MemoryAllocatorBase* allocator()
			{ return _allocator; }

		const MemoryAllocatorBase* allocator() const
			{ return _allocator; }
		
		template <typename O, typename ... args>
		O* createObject(args&&...a)
		{
			ET_ASSERT(_allocator != nullptr);
			return new (_allocator->allocate(sizeof(O))) O(a...);\
		}
		
		template <typename O>
		void deleteObject(O* obj)
		{
			ET_ASSERT(_allocator != nullptr);
			if (obj != nullptr)
			{
#			if (ET_DEBUG)
				_allocator->validatePointer(obj);
#			endif
				
				obj->~O();
				_allocator->release(obj);
			}
		}
		
	private:
		ET_DENY_COPY(ObjectFactory)
		
	private:
		MemoryAllocatorBase* _allocator = nullptr;
	};
	
	class DefaultMemoryAllocator : public MemoryAllocatorBase
	{
		void* release(size_t sz)
			{ return malloc(sz); }
		
		void release(void* ptr)
			{ ::free(ptr); }
		
		bool validatePointer(void*, bool = true)
			{ return true; }
		
		void printInfo() const
			{ log::info("Not available for DefaultMemoryAllocator"); }
	};
	
	class BlockMemoryAllocatorPrivate;
	class BlockMemoryAllocator : public MemoryAllocatorBase
	{
	public:
		BlockMemoryAllocator();
		~BlockMemoryAllocator();
		
		void* allocate(size_t);
		void release(void* ptr);

		bool validatePointer(void*, bool = true);

		void printInfo() const;
		
		void flushUnusedBlocks();
				
	private:
		BlockMemoryAllocatorPrivate* _private = nullptr;
		char _privateData[256];
	};
	
}
