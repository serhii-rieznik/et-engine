/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/containersbase.h>

namespace et
{
	template <typename T>
	class DataStorage : public ContainerBase<T>
	{
	public:
		typedef T DataFormat;

		DataStorage() :  _mutableData(nullptr), _size(0), _dataSize(0), _lastElementIndex(0),
			_flags(DataStorageFlag_OwnsMutableData) { }
		
		explicit DataStorage(size_t size) : _mutableData(nullptr), _size(0), _dataSize(0),
			_lastElementIndex(0), _flags(DataStorageFlag_OwnsMutableData) { resize(size); }

		explicit DataStorage(int size) : _mutableData(nullptr), _size(0), _dataSize(0),
			_lastElementIndex(0), _flags(DataStorageFlag_OwnsMutableData) { resize(static_cast<size_t>(size)); }
		
		DataStorage(size_t size, int initValue) : _mutableData(nullptr), _size(0), _dataSize(0),
			_lastElementIndex(0), _flags(DataStorageFlag_OwnsMutableData)
		{
			resize(size); 
			fill(initValue);
		}

		DataStorage(int size, int initValue) : _mutableData(nullptr), _size(0), _dataSize(0),
			_lastElementIndex(0), _flags(DataStorageFlag_OwnsMutableData)
		{
			resize(static_cast<size_t>(size));
			fill(initValue);
		}
		
		DataStorage(const DataStorage& copy) : _mutableData(nullptr), _size(0), _dataSize(0),
			_lastElementIndex(0), _flags(DataStorageFlag_OwnsMutableData)
		{
			resize(copy.size());
			if (copy.size() > 0)
				etCopyMemory(_mutableData, copy.data(), copy.dataSize());
		}
		
		DataStorage(T* data, size_t dataSize) : _mutableData(data), _size(dataSize / sizeof(T)),
			_dataSize(dataSize), _lastElementIndex(0), _flags(DataStorageFlag_Mutable) { }

		DataStorage(const T* data, size_t dataSize) : _immutableData(data), _size(dataSize / sizeof(T)),
			_dataSize(dataSize), _lastElementIndex(0), _flags(0) { }
		
		~DataStorage()
			{ resize(0); }

	public:
		/*
		 * assignment
		 */
		DataStorage& operator = (const DataStorage& buf)
		{
			if (buf.ownsData())
			{
				_lastElementIndex = buf._lastElementIndex;
				_flags = buf._flags;
				resize(buf.size());
				if (buf.size() > 0)
				{
					etCopyMemory(_mutableData, buf.data(), buf.dataSize());
				}
			}
			else
			{
				_lastElementIndex = 0;
				_mutableData = buf._mutableData;
				_dataSize = buf._dataSize;
				_flags = buf._flags;
				_size = buf._size;
			}
			return *this;
		}
		
	public:
		/*
		 * mutable accessors
		 */
		T* data()
			{ ET_ASSERT(mutableData()); return _mutableData; }
		
		char* binary()
			{ ET_ASSERT(mutableData()); return reinterpret_cast<char*>(_mutableData); }
		
		T& operator [] (int aIndex)
			{ ET_ASSERT(mutableData() && (aIndex >= 0) && (aIndex < static_cast<int>(_size))); return _mutableData[aIndex]; }
		
		T& operator [] (size_t aIndex)
			{ ET_ASSERT(mutableData() && (aIndex < _size)); return _mutableData[aIndex]; }
		
		T& current()
			{ ET_ASSERT(mutableData() && (_lastElementIndex < _size)); return _mutableData[_lastElementIndex]; }

		T* current_ptr()
			{ ET_ASSERT(mutableData() && (_lastElementIndex < _size)); return _mutableData + _lastElementIndex; }
		
		T* element_ptr(size_t aIndex)
			{ ET_ASSERT(aIndex < _size); return (_mutableData + aIndex); }
		
		T* begin()
			{ ET_ASSERT(mutableData()); return _mutableData; }

		T* end()
			{ ET_ASSERT(mutableData()); return _mutableData + _size; }
					 
		/*
		 * const accessors
		 */
		const T* data() const
			{ return _immutableData; }

		const char* binary() const
			{ return reinterpret_cast<const char*>(_immutableData); }

		const T& operator [] (int i) const
			{ ET_ASSERT((i >= 0) && (i < static_cast<int>(_size))); return _immutableData[i]; }

		const T& operator [] (size_t i) const
			{ ET_ASSERT(i < _size); return _immutableData[i]; }
		
		const T& current() const
			{ ET_ASSERT(_lastElementIndex < _size); return _immutableData[_lastElementIndex]; }

		const T* current_ptr() const
			{ ET_ASSERT(_lastElementIndex < _size); return _immutableData + _lastElementIndex; }
		
		const T* element_ptr(size_t i) const
			{ ET_ASSERT(i < _size); return _immutableData + i; }
		
		const T* begin() const
			{ return _immutableData; }
				  
		const T* end() const
			{ return _immutableData + _size; }
					 
		const size_t size() const
			{ return _size; }
		
		const size_t dataSize() const
			{ return _dataSize; }
		
		/*
		 * wrappers
		 */
		char* mutableBinaryData()
			{ return binary(); }
		
		const char* constBinaryData() const
			{ return binary(); }
		
		/*
		 * modifiers
		 */
		void fill(int value)
			{ ET_ASSERT(mutableData()); etFillMemory(_mutableData, value, _dataSize); }
		
		void resize(size_t size)
		{
			if (_size == size) return;
			
			T* new_data = nullptr;
			size_t min_size = (size < _size) ? size : _size;
			_size = size;
			_dataSize = _size * sizeof(T);
			
			if (size > 0)
			{
				new_data = new T[size];
				if (min_size > 0)
					etCopyMemory(new_data, _immutableData, min_size * sizeof(T));
			}
			else
			{
				_lastElementIndex = 0;
			}
			
			if (ownsData())
				delete [] _mutableData;
			
			_flags |= DataStorageFlag_Mutable;
			_mutableData = new_data;
		}
		
		void push_back(const T& value)
		{
			ET_ASSERT(mutableData());
			ET_ASSERT((_lastElementIndex < _size) && "Do no use push back to increase capacity of DataStorage");
			_mutableData[_lastElementIndex++] = value;
		}
		
		void append(const T* values, size_t count)
		{
			ET_ASSERT(mutableData());
			
			size_t currentSize = _size;
			resize(_size + count);
			etCopyMemory(&_mutableData[currentSize], values, count * sizeof(T));
		}
		
		void appendData(const void* ptr, size_t dataSize)
		{
			ET_ASSERT(mutableData());
			ET_ASSERT(ptr);
			
			size_t currentSize = _size;
			size_t numElements = dataSize / sizeof(T) + ((dataSize % sizeof(T) > 0) ? 1 : 0);
			resize(_size + numElements);
			etCopyMemory(&_mutableData[currentSize], ptr, dataSize);
		}
		
		T* extract()
		{
			T* value = _mutableData;
			_mutableData = nullptr;
			_size = 0;
			_dataSize = 0;
			_lastElementIndex = 0;
			return value;
		}

		void fitToSize(size_t size)
		{
			size_t need_size = _lastElementIndex + size;
			if (need_size > _size)
				resize(need_size);
		}

		const size_t lastElementIndex() const
			{ return _lastElementIndex; }
		
		void applyOffset(size_t o)
			{ ET_ASSERT(mutableData()); _lastElementIndex += o; }

		void setOffset(size_t o) 
			{ ET_ASSERT(mutableData()); _lastElementIndex = o; }

	private:
		enum
		{
			DataStorageFlag_OwnsData = 0x01,
			DataStorageFlag_Mutable = 0x02,
			DataStorageFlag_OwnsMutableData = DataStorageFlag_OwnsData | DataStorageFlag_Mutable,
		};
		
		bool ownsData() const
			{ return (_flags & DataStorageFlag_OwnsData); }
		
		bool mutableData() const
			{ return (_flags & DataStorageFlag_Mutable) != 0; }
		
	private:
		union
		{
			T* _mutableData;
			const T* _immutableData;
		};
		
	private:
		size_t _size;
		size_t _dataSize;
		size_t _lastElementIndex;
		size_t _flags;
	};

	typedef DataStorage<float> FloatDataStorage;
	typedef DataStorage<unsigned char> BinaryDataStorage;
	typedef DataStorage<char> StringDataStorage;
}
