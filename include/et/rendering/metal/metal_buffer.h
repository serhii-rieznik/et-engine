/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#if !defined(__OBJC__)
#	error Do not attempt to include this file from cpp
#endif

#include <et/rendering/metal/metal.h>

namespace et
{
	class MetalBuffer
	{
	public:
		MetalBuffer() = default;
		MetalBuffer(MetalState& metal, const void* data, uint32_t size);

		~MetalBuffer();

		id<MTLBuffer> buffer()
			{ return _buffer; }

	private:
		id<MTLBuffer> _buffer = nil;
	};

	inline MetalBuffer::MetalBuffer(MetalState& metal, const void* data, uint32_t size)
	{
		_buffer = [metal.device newBufferWithBytes:data length:size options:MTLResourceCPUCacheModeDefaultCache];
	}

	inline MetalBuffer::~MetalBuffer()
	{
		ET_OBJC_RELEASE(_buffer);
	}
}
