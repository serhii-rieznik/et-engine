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
	template <typename T>
	class RawDataAcessor : public ContainerBase<T>
	{
	public:
		enum
		{
			TypeSize = sizeof(T)
		};
		
	public:
		RawDataAcessor()
			{ }
		
		RawDataAcessor(char* data, const size_t dataSize, const size_t stride, const size_t offset) :
			_mutableData(data), _mutable(true)
		{
			_offset = offset;
			_stride = stride ? stride : TypeSize;
			_dataSize = (dataSize / _stride) * _stride;
			_size = _dataSize / _stride;
		}

		RawDataAcessor(const char* data, const size_t dataSize, const size_t stride, const size_t offset) :
			_immutableData(data), _mutable(false)
		{
			_offset = offset;
			_stride = stride ? stride : TypeSize;
			_dataSize = (dataSize / _stride) * _stride;
			_size = _dataSize / _stride;
		}
		
		/* 
		 * mutable accessors
		 */
		T& operator [] (uint32_t i)
			{ ET_ASSERT(_mutable); ET_ASSERT(i < _size); return *(reinterpret_cast<T*>(_mutableData + i * _stride + _offset)); }
		
		T& operator [] (int32_t i)
			{ ET_ASSERT(_mutable); ET_ASSERT(i >= 0); return operator [] (static_cast<uint32_t>(i)); }
		
		void fill(int v)
			{ ET_ASSERT(_mutable); etFillMemory(_mutableData, v, _dataSize); }
		
		char* binary()
			{ ET_ASSERT(_mutable); return (char*)_mutableData; };
		
		/*
		 * immutable accessors
		 */
		const T& operator [] (uint32_t i) const
			{ ET_ASSERT(i < _size); return *(reinterpret_cast<const T*>(_immutableData + i * _stride + _offset)); }
		
		const T& operator [] (int32_t i) const
			{ ET_ASSERT(i >= 0); return operator [] (static_cast<uint32_t>(i)); }

		const char* binary() const
			{ return (char*)_immutableData; };

		/*
		 * other accessors
		 */
		size_t size() const
			{ return _size; };
		
		size_t dataSize() const
			{ return _dataSize; }

		bool valid() const
			{ return (_immutableData != nullptr); }

	public:
		RawDataAcessor& operator = (const RawDataAcessor& r)
		{
			_dataSize = r._dataSize;
			_size = r._size;
			_stride = r._stride;
			_offset = r._offset;
			_immutableData = r._immutableData;
			_mutable = r._mutable;
			return *this;
		}

	private:
		size_t _dataSize = 0;
		size_t _size = 0;
		size_t _stride = 0;
		size_t _offset = 0;
		union
		{
			char* _mutableData = nullptr;
			const char* _immutableData;
		};
		bool _mutable = false;
	};

}
