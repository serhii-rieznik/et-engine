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
	class DataStorage : public ContainerBase<T>
	{
	public:
		typedef T DataType;
		
		typedef T* DataTypePointer;
		typedef const T* DataTypeConstPointer;
		
		typedef T& DataTypeReference;
		typedef const T& DataTypeConstReference;
		
		enum
		{
			dataTypeSize = sizeof(DataType)
		};
		
	public:
		DataStorage() :
			_mutableData(nullptr), _flags(DataStorageFlag_OwnsMutableData) { }
		
		explicit DataStorage(size_t size) :
			_mutableData(nullptr), _flags(DataStorageFlag_OwnsMutableData)
		{
			resize(size);
		}

		DataStorage(size_t size, int initValue) :
			_mutableData(nullptr), _flags(DataStorageFlag_OwnsMutableData)
		{
			resize(size); 
			fill(initValue);
		}

		DataStorage(const DataStorage& copy) :
			_mutableData(nullptr), _flags(DataStorageFlag_OwnsMutableData)
		{
			resize(copy.size());
			
			if (copy.size() > 0)
				etCopyMemory(_mutableData, copy.data(), copy.dataSize());
		}
		
		DataStorage(DataStorage&& mv)
		{
			_size = mv._size;
			_dataSize = mv._dataSize;
			_lastElementIndex = mv._lastElementIndex;
			_flags = mv._flags;
			_mutableData = mv._mutableData;
			_immutableData = mv._immutableData;

			mv._size = 0;
			mv._dataSize = 0;
			mv._lastElementIndex = 0;
			mv._flags = 0;
			mv._mutableData = nullptr;
			mv._immutableData = nullptr;
		}
		
		DataStorage(DataTypePointer data, size_t dataSize) :
			_mutableData(data), _size(dataSize / dataTypeSize), _dataSize(dataSize),
			_flags(DataStorageFlag_Mutable) { }

		DataStorage(DataTypeConstPointer data, size_t dataSize) :
			_immutableData(data), _size(dataSize / dataTypeSize), _dataSize(dataSize) { }

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
		DataTypePointer data()
			{ ET_ASSERT(mutableData()); return _mutableData; }
		
		char* binary()
			{ ET_ASSERT(mutableData()); return reinterpret_cast<char*>(_mutableData); }
		
		DataTypeReference operator [] (int32_t aIndex)
			{ ET_ASSERT(mutableData() && (aIndex >= 0) && (aIndex < static_cast<int>(_size))); return _mutableData[aIndex]; }
		
		DataTypeReference operator [] (uint32_t aIndex)
			{ ET_ASSERT(mutableData() && (aIndex < _size)); return _mutableData[aIndex]; }
		
		DataTypeReference current()
			{ ET_ASSERT(mutableData() && (_lastElementIndex < _size)); return _mutableData[_lastElementIndex]; }

		DataTypePointer current_ptr()
			{ ET_ASSERT(mutableData() && (_lastElementIndex < _size)); return _mutableData + _lastElementIndex; }
		
		DataTypePointer element_ptr(size_t aIndex)
			{ ET_ASSERT(aIndex < _size); return (_mutableData + aIndex); }
		
		DataTypePointer begin()
			{ ET_ASSERT(mutableData()); return _mutableData; }

		DataTypePointer end()
			{ ET_ASSERT(mutableData()); return _mutableData + _size; }
					 
		/*
		 * const accessors
		 */
		DataTypeConstPointer data() const
			{ return constData(); }

		DataTypeConstPointer constData() const
			{ return _immutableData; }
		
		DataTypeConstReference operator [] (int32_t i) const
			{ ET_ASSERT((i >= 0) && (i < static_cast<int>(_size))); return _immutableData[i]; }

		DataTypeConstReference operator [] (uint32_t i) const
			{ ET_ASSERT(i < _size); return _immutableData[i]; }
		
		DataTypeConstReference current() const
			{ ET_ASSERT(_lastElementIndex < _size); return _immutableData[_lastElementIndex]; }

		DataTypeConstPointer current_ptr() const
			{ ET_ASSERT(_lastElementIndex < _size); return _immutableData + _lastElementIndex; }
		
		DataTypeConstPointer element_ptr(size_t i) const
			{ ET_ASSERT(i < _size); return _immutableData + i; }
		
		DataTypeConstPointer begin() const
			{ return _immutableData; }
				  
		DataTypeConstPointer end() const
			{ return _immutableData + _size; }
		
		const char* binary() const
			{ return reinterpret_cast<const char*>(_immutableData); }
		
		size_t size() const
			{ return _size; }
		
		size_t dataSize() const
			{ return _dataSize; }

		/*
		 * unsafe access 
		 */
		DataTypePointer mutableDataUnsafe()
			{ return _mutableData; }

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
		
		void resize(size_t newSize)
		{
			if (_size == newSize) return;
			
			DataTypePointer new_data = nullptr;
			size_t min_size = (newSize < _size) ? newSize : _size;
			_size = newSize;
			_dataSize = _size * dataTypeSize;
			
			if (newSize > 0)
			{
				new_data = reinterpret_cast<DataTypePointer>(sharedBlockAllocator().allocate(_dataSize));
				if (min_size > 0)
					etCopyMemory(new_data, _immutableData, min_size * dataTypeSize);
			}
			else
			{
				_lastElementIndex = 0;
			}
			
			if (ownsData())
				sharedBlockAllocator().release(_mutableData);
			
			_flags |= DataStorageFlag_Mutable;
			_mutableData = new_data;
		}
		
		void push_back(DataTypeConstReference value)
		{
			ET_ASSERT(mutableData());
			ET_ASSERT((_lastElementIndex < _size) && "Do no use push back to increase capacity of DataStorage");
			_mutableData[_lastElementIndex++] = value;
		}
		
		void append(DataTypeConstPointer values, size_t count)
		{
			ET_ASSERT(mutableData());
			
			size_t currentSize = _size;
			resize(_size + count);
			etCopyMemory(&_mutableData[currentSize], values, count * dataTypeSize);
		}
		
		void appendData(const void* ptr, size_t dataSize)
		{
			ET_ASSERT(mutableData());
			ET_ASSERT(ptr);
			
			size_t currentSize = _size;
			size_t numElements = dataSize / dataTypeSize + ((dataSize % dataTypeSize > 0) ? 1 : 0);
			resize(_size + numElements);
			etCopyMemory(&_mutableData[currentSize], ptr, dataSize);
		}
		
		DataTypePointer extract()
		{
			DataTypePointer value = _mutableData;
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

		uint32_t lastElementIndex() const
			{ return _lastElementIndex; }
		
		void applyOffset(uint32_t o)
			{ ET_ASSERT(mutableData()); _lastElementIndex += o; }

		void setOffset(uint32_t o)
			{ ET_ASSERT(mutableData()); _lastElementIndex = o; }
		
	public:
		/*
		 * Filesystem
		 */
		bool writeToFile(const std::string& fileName, bool atomically = true) const
		{
			auto targetFileName = fileName;
			
			if (atomically)
			{
				int64_t hash = std::time(nullptr) ^ reinterpret_cast<int64_t>(&fileName);
				targetFileName += intToStr(hash ^ 0xdeadbeefaaaaaaaa);
			}

			std::ofstream fOut(targetFileName, std::ios::out | std::ios::binary);
			
			if (fOut.fail())
				return false;
			
			fOut.write(binary(), _dataSize);
			fOut.flush();
			fOut.close();
			
			if (atomically)
			{
				if (::rename(targetFileName.c_str(), fileName.c_str()))
				{
					if (::remove(fileName.c_str()))
						return false;
					
					if (::rename(targetFileName.c_str(), fileName.c_str()))
						return false;
				}
			}
			
			return true;
		}

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
			DataTypePointer _mutableData = nullptr;
			DataTypeConstPointer _immutableData;
		};
		
	private:
		size_t _size = 0;
		size_t _dataSize = 0;
		uint32_t _lastElementIndex = 0;
		uint32_t _flags = 0;
	};

	typedef DataStorage<float> FloatDataStorage;
	typedef DataStorage<unsigned char> BinaryDataStorage;
	typedef DataStorage<char> StringDataStorage;
}
