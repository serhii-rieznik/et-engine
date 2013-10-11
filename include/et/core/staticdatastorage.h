/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
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
			{ assert(i < count); return data[i]; }
		
		const T& operator [](size_t i) const
			{ assert(i < count); return data[i]; }

		char* binary()
			{ return reinterpret_cast<char*>(data); }
		
		const char* binary() const
			{ return reinterpret_cast<const char*>(data); }

		const size_t size() const
			{ return count; }
		
		const size_t dataSize() const 
			{ return count * sizeof(T); }
		
		void copyFrom(const StaticDataStorage& r)
		{
			assert(r.size() == size() && "Can not copy from array with different size");
			
			size_t i = 0;
			for (auto& value : r.data)
				data[i++] = value;
		}
		
		T* begin()
			{ return data; }
		
		T* end()
			{ return data + count; }
	};
}
