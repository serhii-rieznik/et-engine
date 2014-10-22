//
//  MemoryAllocator.h
//  etConsoleApplication
//
//  Created by Sergey Reznik on 18/10/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <cstddef>

namespace et
{
	class MemoryAllocatorPrivate;
	class MemoryAllocator
	{
	public:
		MemoryAllocator();
		
		~MemoryAllocator();
		
		void* alloc(size_t);
		void free(void*);
		
		template <typename T, typename ... args>
		T* createObject(args&&...a)
		{
			return new (alloc(sizeof(T))) T(a...);
		}
		
		template <typename T>
		void deleteObject(T* t)
		{
			t->~T();
			free(t);
		}
		
		void printInfo();
				
	private:
		enum
		{
			PrivateEstimatedSize = 32,
		};
		
		MemoryAllocator(const MemoryAllocator&) = delete;
		MemoryAllocator(MemoryAllocator&&) = delete;
		MemoryAllocator& operator = (const MemoryAllocator&) = delete;
		
	private:
		MemoryAllocatorPrivate* _private = nullptr;
		char _privateData[PrivateEstimatedSize];
	};
}