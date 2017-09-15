/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/et.h>

namespace et
{

template <uint32_t size>
struct PoolObject
{
	uint8_t data[size] { };
	uint32_t pageIndex = 0;
	uint32_t allocated = 0;
};

template <class T>
class PoolPointer
{
	static_assert(std::is_base_of<Shared, T>::value,
		"Pool pointer content must be derived from Shared class");

	static_assert(offsetof(PoolObject<sizeof(T)>, data) == 0,
		"Data field in PoolObject should have zero offset");

public:
	PoolPointer() {
	}

	template <class ... Args>
	PoolPointer(PoolObject<sizeof(T)>* storage, Args&&... args) {
		reset(new (storage->data) T(std::forward<Args>(args)...));
	}

	~PoolPointer() {
		reset(nullptr);
	}

	PoolPointer(const PoolPointer& r) {
		reset(r._object);
	}

	PoolPointer(PoolPointer&& r) 
		: _object(r._object)
	{
		r._object = nullptr;
	}

	PoolPointer& operator = (const PoolPointer& r) {
		reset(r._object);
		return *this;
	}

	void reset() {
		reset(nullptr);
	}

	bool invalid() const {
		return _object == nullptr;
	}
	
	bool valid() const {
		return _object != nullptr;
	}

	T* operator -> () { 
		ET_ASSERT(_object != nullptr); 
		return _object; 
	}
	
	const T* operator -> () const { 
		ET_ASSERT(_object != nullptr); 
		return _object;
	}

	template <class ... Args>
	static PoolPointer<T> create(Args&&...);

private:
	void destroy(T* object)
	{
		PoolObject<sizeof(T)>* poolObject = reinterpret_cast<PoolObject<sizeof(T)>*>(object);
		object->~T();
		object = nullptr;

		memset(poolObject->data, 0x0, sizeof(T));
		poolObject->allocated = 0;
	}

	void reset(T* newObject)
	{
		if (newObject == _object)
			return;

		if (newObject != nullptr)
			newObject->retain();

		T* oldObject = _object;
		std::swap(_object, newObject);

		if (oldObject != nullptr)
		{
			ET_ASSERT(oldObject->retainCount() > 0);
			if (oldObject->release() == 0)
				destroy(oldObject);
		}
	}

private:
	T* _object = nullptr;
};

template <class T, uint32_t pageSize>
class Pool
{
public:
	Pool() {
		allocateStorage();
	}

	~Pool() {
		cleanup();
	}

	template <class ... Args>
	PoolPointer<T> acquire(Args&&... args)
	{
		PoolObject<sizeof(T)>* entry = nullptr;

		for (Page* page : pages)
		{
			auto begin = page->data();
			auto end = begin + pageSize;
			for (; begin < end; ++begin)
			{
				if (begin->allocated == 0)
				{
					entry = begin;
					entry->allocated = 1;
					break;
				}
			}
			
			if (entry != nullptr)
				break;
		}

		return PoolPointer<T>(entry, std::forward<Args>(args)...);
	}

	void cleanup() 
	{
		for (Page* page : pages) 
			sharedBlockAllocator().release(page);

		pages.clear();
	}

private:
	using Page = std::array<PoolObject<sizeof(T)>, pageSize>;

	enum : uint64_t { 
		PageDataSize = sizeof(Page)
	};

private:
	void allocateStorage()
	{
		void* ptr = sharedBlockAllocator().allocate(PageDataSize);
		memset(ptr, 0, PageDataSize);
		pages.emplace_back(reinterpret_cast<Page*>(ptr));
	}

private:
	Vector<Page*> pages;
};

template <class T> 
template <class ... Args>
inline PoolPointer<T> PoolPointer<T>::create(Args&&...args) {
	return T::pool().acquire(std::forward<Args>(args)...);
}

#define ET_DECLARE_POOL_POINTER(T) static Pool<T, 1024>& pool(); using Pointer = PoolPointer<T>
#define ET_IMPLEMENT_POOL(T) Pool<T, 1024>& T::pool() { static Pool<T, 1024> _pool; return _pool; }

}
