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

class SharedConstBufferPrivate;
class SharedConstBuffer
{
public:
	enum
	{
		Capacity = 16 * 1024 * 1024,
		Granularity = 512
	};

public:
	SharedConstBuffer();
	~SharedConstBuffer();

	void init(RenderInterface*);
	void shutdown();

	DataBuffer::Pointer buffer() const;
	void flush();

	uint8_t* staticAllocate(uint32_t size, uint32_t& bufferOffset);
	uint8_t* dynamicAllocate(uint32_t size, uint32_t& bufferOffset);
	
	void free(uint8_t*);

private:
	ET_DECLARE_PIMPL(SharedConstBuffer, 256);
};

}
