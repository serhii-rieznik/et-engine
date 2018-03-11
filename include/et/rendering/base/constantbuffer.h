/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/camera/camera.h>
#include <et/rendering/interface/buffer.h>

namespace et
{

enum : uint32_t
{
	ConstantBufferStaticAllocation = 1 << 1,
	ConstantBufferDynamicAllocation = 1 << 2,
	InvalidFlushFrame = std::numeric_limits<uint32_t>::max()
};

class ConstantBufferEntry : public Object
{
public:
	ET_DECLARE_POINTER(ConstantBufferEntry);

public:
	ConstantBufferEntry(uint64_t off, uint64_t sz, uint8_t* ptr, uint32_t aCl) :
		_data(ptr), _allocationClass(aCl), _offset(off), _length(sz) { }

	uint64_t offset() const
		{ return _offset; }
	
	uint64_t length() const 
		{ return _length; }

	uint8_t* data() 
		{ return _data; }
	
	const uint8_t* data() const 
		{ return _data; }

	bool valid() const
		{ return (_length > 0) && (_data != nullptr); }

	uint32_t allocationClass() const 
		{ return _allocationClass; }

	uint64_t flushFrame() const 
		{ return _flushFrame;  }

	void flush(uint64_t frame) 
		{ _flushFrame = frame; }

	bool operator == (const ConstantBufferEntry& r) const 
	{
		return (_data == r._data) && (_offset == r._offset) && (_length == r._length) && 
			(_allocationClass == r._allocationClass);
	}

private:
	uint8_t* _data = nullptr;
	uint64_t _offset = 0;
	uint64_t _length = 0;
	uint32_t _allocationClass = 0;
	uint64_t _flushFrame = InvalidFlushFrame;
};

class RenderInterface;
class ConstantBufferPrivate;
class ConstantBuffer
{
public:
	enum
	{
		Capacity = 16 * 1024 * 1024,
		Granularity = 256,
	};

public:
	ConstantBuffer();
	~ConstantBuffer();

	void init(RenderInterface*, uint32_t allowedAllocations);
	void shutdown();

	Buffer::Pointer buffer() const;
	void flush(uint64_t);

	const ConstantBufferEntry::Pointer& allocate(uint64_t size, uint32_t allocationClass);

private:
	ET_DECLARE_PIMPL(ConstantBuffer, 384);
};

}
