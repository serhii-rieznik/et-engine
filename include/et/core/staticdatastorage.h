/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containersbase.h>

namespace et
{
	template <typename T, size_t count>
	class StaticDataStorage : public ContainerBase<T>
	{
	public:
		StaticDataStorage()
			{ }
		
		StaticDataStorage(int initialize) 
			{ etFillMemory(data, initialize, sizeof(data)); }
		
		StaticDataStorage(StaticDataStorage&& r)
			{ copyFrom(r); }

		StaticDataStorage(const StaticDataStorage& r)
			{ copyFrom(r); }
		
		StaticDataStorage& operator = (const StaticDataStorage& r)
			{ copyFrom(r); return *this; }

		T data[count];

		void fill(int value)
			{ etFillMemory(data, value, dataSize()); }

		T& operator [](size_t i)
			{ ET_ASSERT(i < count); return data[i]; }
		
		const T& operator [](size_t i) const
			{ ET_ASSERT(i < count); return data[i]; }

		char* binary()
			{ return reinterpret_cast<char*>(data); }
		
		const char* binary() const
			{ return reinterpret_cast<const char*>(data); }

		size_t size() const
			{ return count; }
		
		size_t dataSize() const 
			{ return count * sizeof(T); }
		
		void copyFrom(const StaticDataStorage& r)
		{
			ET_ASSERT(r.size() == size() && "Can not copy from array with different size");
			
			size_t i = 0;
			for (auto& value : r.data)
				data[i++] = value;
		}

		const T* begin() const
			{ return data; }
		
		T* begin()
			{ return data; }

		const T* end() const
			{ return data + count; }
		
		T* end()
			{ return data + count; }
	};
}
