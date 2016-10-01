/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/databuffer.h>

namespace et
{
class RenderInterface;
class ConstBuffer
{
public:
	enum : uint32_t
	{
		Capacity = 4 * 1024 * 1024,
		Alignment = 256
	};

public:
	void init(RenderInterface*);
	uint8_t* allocateData(uint32_t size, uint32_t& baseOffset);
	void flush();
	void reset();

	DataBuffer::Pointer buffer() const
		{ return _buffer; }

private:
	std::mutex _lock;
	DataBuffer::Pointer _buffer;
	BinaryDataStorage _localData;
	uint32_t _offset = 0;
	uint32_t _startOffset = 0;
};

}
