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
class Buffer : public NamedObject
{
public:
	ET_DECLARE_POINTER(Buffer);

	struct Range
	{
		uint64_t begin = 0;
		uint64_t length = 0;
		Range(uint64_t b, uint64_t l) : 
			begin(b), length(l) { }
	};

	enum class Usage : uint32_t
	{
		Constant,
		Vertex,
		Index,
		Staging
	};

	enum class Location : uint32_t
	{
		Device,
		Host
	};

	struct Description
	{
		uint64_t size = 0;
		uint64_t alignedSize = 0;
		Usage usage = Usage::Constant;
		Location location = Location::Device;
		BinaryDataStorage initialData;
	};

public:
	Buffer() = default;
	virtual ~Buffer() = default;

	virtual uint8_t* map(uint64_t begin, uint64_t length) = 0;
	virtual void modifyRange(uint64_t begin, uint64_t length) = 0;
	virtual void unmap() = 0;

	virtual bool mapped() const = 0;

	virtual void updateData(uint64_t offset, const BinaryDataStorage&) = 0;
	virtual void transferData(Buffer::Pointer destination, uint64_t srcOffset, uint64_t dstOffset, uint64_t size) = 0;

	virtual uint64_t size() const = 0;
};

}
