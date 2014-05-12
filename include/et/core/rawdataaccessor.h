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
	template <typename T>
	class RawDataAcessor : public ContainerBase<T>
	{
	public:
		static const int TypeSize = sizeof(T);

		RawDataAcessor() :
			_dataSize(0), _size(0), _stride(0), _offset(0), _data(nullptr) { }

		RawDataAcessor(char* data, const size_t dataSize, const size_t stride, const size_t offset) :
			_data(data)
		{ 
			size_t estimatedDataSize = dataSize - offset;

			_offset = offset;
			_stride = stride ? stride : TypeSize;
			_dataSize = (estimatedDataSize / _stride) * _stride;
			_size = _dataSize / _stride;
		}

		T& operator [] (size_t i)
			{ ET_ASSERT(i < _size); return *(reinterpret_cast<T*>(_data + i * _stride + _offset)); }

		T& operator [] (int i)
			{ return operator [] (static_cast<size_t>(i)); }
		
		void fill(int v)
			{ etFillMemory(_data, v, _dataSize); }

		char* binary()
			{ return (char*)_data; };
		
		const char* binary() const
			{ return (char*)_data; };

		const size_t size() const
			{ return _size; };
		
		const size_t dataSize() const
			{ return _dataSize; }

		bool valid() const
			{ return _data != nullptr; }

		RawDataAcessor& operator = (const RawDataAcessor& r)
		{
			_dataSize = r._dataSize;
			_size = r._size;
			_stride = r._stride;
			_offset = r._offset;
			_data = r._data;
			return *this;
		}

	private:
		size_t _dataSize;
		size_t _size;
		size_t _stride;
		size_t _offset;
		char* _data;
	};

}
