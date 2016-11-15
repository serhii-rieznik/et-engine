/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>

namespace et
{
class DataBuffer : public Object
{
public:
	ET_DECLARE_POINTER(DataBuffer);

	struct Range
	{
		uint32_t begin = 0;
		uint32_t length = 0;
		
		Range() = default;
		Range(uint32_t b, uint32_t l) : 
			begin(b), length(l) { }
	};

public:
	virtual uint32_t size() const = 0;
	virtual void setData(const void* ptr, uint32_t offset, uint32_t size) = 0;

	virtual uint8_t* map(uint32_t offset, uint32_t size) = 0;
	virtual void unmap() = 0;

	virtual void flushRanges(const Vector<Range>&) = 0;
};
}
