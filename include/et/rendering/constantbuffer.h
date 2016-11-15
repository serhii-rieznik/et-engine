/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/camera/camera.h>
#include <et/rendering/interface/databuffer.h>

namespace et
{

class ConstantBufferEntry
{
public:
	ConstantBufferEntry() = default;

	ConstantBufferEntry(uint32_t off, uint32_t sz, uint8_t* ptr, bool dyn) :
		_data(ptr), _offset(off), _length(sz), _isDynamic(dyn) { }

	uint32_t offset() const
		{ return _offset; }
	
	uint32_t length() const 
		{ return _length; }

	uint8_t* data() 
		{ return _data; }
	
	const uint8_t* data() const 
		{ return _data; }

	bool valid() const
		{ return (_length > 0) && (_data != nullptr); }

	bool isDynamic() const 
		{ return _isDynamic; }

private:
	uint8_t* _data = nullptr;
	uint32_t _offset = 0;
	uint32_t _length = 0;
	bool _isDynamic = false;
};

class RenderInterface;
class ConstantBufferPrivate;
class ConstantBuffer
{
public:
	enum
	{
		Capacity = 16 * 1024 * 1024,
		Granularity = 256
	};

public:
	ConstantBuffer();
	~ConstantBuffer();

	void init(RenderInterface*);
	void shutdown();

	DataBuffer::Pointer buffer() const;
	void flush();

	ConstantBufferEntry staticAllocate(uint32_t size);
	ConstantBufferEntry dynamicAllocate(uint32_t size);

	void staticFree(const ConstantBufferEntry&);
	void dynamicFree(const ConstantBufferEntry&);

private:
	ET_DECLARE_PIMPL(ConstantBuffer, 256);
};

}
