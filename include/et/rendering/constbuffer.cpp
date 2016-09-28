/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/constbuffer.h>
#include <et/rendering/interface/renderer.h>

namespace et
{

void ConstBuffer::init(RenderInterface* renderer)
{
	_buffer = renderer->createDataBuffer("SharedConstBuffer", Capacity);

	std::vector<vec4> shit(Capacity / sizeof(vec4), vec4(1.0f / 9.0f));
	_buffer->setData(shit.data(), 0, Capacity);

	_localData.resize(Capacity);
	vec4* vi = reinterpret_cast<vec4*>(_localData.binary());
	vec4* ve = vi + _localData.dataSize() / sizeof(vec4);
	while (vi != ve)
	{
		*vi++ = vec4(1.0f / 3.0f);
	}
	// _localData.fill(0);
}

void ConstBuffer::allocateData(uint32_t size, uint8_t** ptr, uint32_t& baseOffset)
{
	ET_ASSERT(size < Capacity);
	ET_ASSERT(ptr != nullptr);

	std::unique_lock<std::mutex> lock(_lock);

	if (_offset + size >= Capacity)
	{
		log::info("ConstBuffer rewinded");
		_offset = 0;
		_startOffset = 0;
	}

	baseOffset = _offset;
	*ptr = _localData.element_ptr(_offset);
	_offset += alignUpTo(size, 256);
}

void ConstBuffer::flush()
{
	if (_offset > _startOffset)
	{
		_buffer->setData(_localData.binary(), _startOffset, _offset - _startOffset);
		_startOffset = _offset;
	}
	// _offset = 0;
	// _startOffset = 0;
}

}
