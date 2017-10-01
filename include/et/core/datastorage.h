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
	enum : uint64_t
	{
		DataTypeSize = sizeof(T)
	};

public:
	DataStorage() :
		_mutableData(nullptr), _flags(DataStorageFlag_OwnsMutableData)
	{
	}

	explicit DataStorage(uint64_t size) :
		_mutableData(nullptr), _flags(DataStorageFlag_OwnsMutableData)
	{
		resize(size);
	}

	DataStorage(uint64_t size, int initValue) :
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

	DataStorage(T* data, uint64_t dataSize) :
		_mutableData(data), _size(dataSize / DataTypeSize), _dataSize(dataSize),
		_flags(DataStorageFlag_Mutable)
	{
	}

	DataStorage(const T* data, uint64_t dataSize) :
		_immutableData(data), _size(dataSize / DataTypeSize), _dataSize(dataSize)
	{
	}

	~DataStorage()
	{
		resize(0);
	}

public:
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
	{
		ET_ASSERT(mutableData()); return _mutableData;
	}

	char* binary()
	{
		ET_ASSERT(mutableData()); return reinterpret_cast<char*>(_mutableData);
	}

	T& operator [] (uint64_t aIndex)
	{
		ET_ASSERT(mutableData() && (aIndex < _size)); return _mutableData[aIndex];
	}

	T& current()
	{
		ET_ASSERT(mutableData() && (_lastElementIndex < _size)); return _mutableData[_lastElementIndex];
	}

	T* current_ptr()
	{
		ET_ASSERT(mutableData() && (_lastElementIndex < _size)); return _mutableData + _lastElementIndex;
	}

	T* element_ptr(uint64_t aIndex)
	{
		ET_ASSERT(aIndex < _size); return (_mutableData + aIndex);
	}

	T* begin()
	{
		ET_ASSERT(mutableData()); return _mutableData;
	}

	T* end()
	{
		ET_ASSERT(mutableData()); return _mutableData + _size;
	}

	/*
	 * const accessors
	 */
	const T* data() const
	{
		return constData();
	}

	const T* constData() const
	{
		return _immutableData;
	}

	const T& operator [] (uint64_t i) const
	{
		ET_ASSERT(i < _size); return _immutableData[i];
	}

	const T& current() const
	{
		ET_ASSERT(_lastElementIndex < _size); return _immutableData[_lastElementIndex];
	}

	const T* current_ptr() const
	{
		ET_ASSERT(_lastElementIndex < _size); return _immutableData + _lastElementIndex;
	}

	const T* element_ptr(uint64_t i) const
	{
		ET_ASSERT(i < _size); return _immutableData + i;
	}

	const T* begin() const
	{
		return _immutableData;
	}

	const T* end() const
	{
		return _immutableData + _size;
	}

	const char* binary() const
	{
		return reinterpret_cast<const char*>(_immutableData);
	}

	uint64_t size() const
	{
		return _size;
	}

	uint64_t dataSize() const
	{
		return _dataSize;
	}

	bool empty() const
	{
		return _size == 0;
	}

	/*
	 * wrappers
	 */
	char* mutableBinaryData()
	{
		return binary();
	}

	const char* constBinaryData() const
	{
		return binary();
	}

	/*
	 * modifiers
	 */
	void fill(int value)
	{
		ET_ASSERT(mutableData()); etFillMemory(_mutableData, value, _dataSize);
	}

	void resize(uint64_t newSize)
	{
		if (_size == newSize) return;

		T* new_data = nullptr;
		uint64_t min_size = (newSize < _size) ? newSize : _size;
		_size = newSize;
		_dataSize = _size * DataTypeSize;

		if (newSize > 0)
		{
			new_data = reinterpret_cast<T*>(sharedBlockAllocator().allocate(_dataSize));
			if (min_size > 0)
				etCopyMemory(new_data, _immutableData, min_size * DataTypeSize);
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

	void push_back(const T& value)
	{
		ET_ASSERT(mutableData());
		ET_ASSERT((_lastElementIndex < _size) && "Do no use push back to increase capacity of DataStorage");
		_mutableData[_lastElementIndex++] = value;
	}

	void append(const T* values, uint64_t count)
	{
		ET_ASSERT(mutableData());

		uint64_t currenDataTypeSize = _size;
		resize(_size + count);
		etCopyMemory(&_mutableData[currenDataTypeSize], values, count * DataTypeSize);
	}

	void appendData(const void* ptr, uint64_t dataSize)
	{
		ET_ASSERT(mutableData());
		ET_ASSERT(ptr);

		uint64_t currenDataTypeSize = _size;
		uint64_t numElements = dataSize / DataTypeSize + ((dataSize % DataTypeSize > 0) ? 1 : 0);
		resize(_size + numElements);
		etCopyMemory(&_mutableData[currenDataTypeSize], ptr, dataSize);
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

	void fitToSize(uint64_t size)
	{
		uint64_t need_size = _lastElementIndex + size;
		if (need_size > _size)
			resize(need_size);
	}

	uint64_t lastElementIndex() const
	{
		return _lastElementIndex;
	}

	void applyOffset(uint64_t o)
	{
		ET_ASSERT(mutableData()); _lastElementIndex += o;
	}

	void setOffset(uint64_t o)
	{
		ET_ASSERT(mutableData()); _lastElementIndex = o;
	}

public:
	/*
	 * Filesystem
	 */
	bool writeToFile(const std::string& fileName, bool atomically = true) const
	{
		auto targetFileName = fileName;

		if (atomically)
		{
			uint64_t hash = static_cast<uint64_t>(std::time(nullptr)) ^ reinterpret_cast<uint64_t>(&fileName);
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
	{
		return (_flags & DataStorageFlag_OwnsData);
	}

	bool mutableData() const
	{
		return (_flags & DataStorageFlag_Mutable) != 0;
	}

private:
	union
	{
		T* _mutableData = nullptr;
		const T* _immutableData;
	};

private:
	uint64_t _size = 0;
	uint64_t _dataSize = 0;
	uint64_t _lastElementIndex = 0;
	uint64_t _flags = 0;
};

typedef DataStorage<float> FloatDataStorage;
typedef DataStorage<uint8_t> BinaryDataStorage;
typedef DataStorage<char> StringDataStorage;
}
