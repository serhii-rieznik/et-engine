/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
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
		
		virtual void* alloc(size_t) = 0;
		virtual void free(void*) = 0;
		
		virtual bool validatePointer(void*, bool abortOnFail = true) = 0;
		
		virtual void printInfo() const = 0;
		
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
			return new (_allocator->alloc(sizeof(O))) O(a...);\
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
				_allocator->free(obj);
			}
		}
		
	private:
		ET_DENY_COPY(ObjectFactory)
		
	private:
		MemoryAllocatorBase* _allocator = nullptr;
	};
	
	class DefaultMemoryAllocator : public MemoryAllocatorBase
	{
		void* alloc(size_t sz)
			{ return malloc(sz); }
		
		void free(void* ptr)
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
		
		void* alloc(size_t);
		void free(void*);
		bool validatePointer(void*, bool = true);
		void printInfo() const;
				
	private:
		BlockMemoryAllocatorPrivate* _private = nullptr;
		char _privateData[256];
	};
	
}
