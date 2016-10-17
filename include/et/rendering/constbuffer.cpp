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
	_localData.fill(0);
}

void ConstBuffer::shutdown()
{

}

uint8_t* ConstBuffer::allocateData(uint32_t size, uint32_t& baseOffset)
{
	ET_ASSERT(size < Capacity);

	std::unique_lock<std::mutex> lock(_lock);
	if (_offset + size >= Capacity)
	{
		flush();
		reset();
		log::info("ConstBuffer rewinded");
	}

	baseOffset = _offset;
	_offset += alignUpTo(size, Alignment);
	return _localData.data() + baseOffset;
}

void ConstBuffer::flush()
{
	if (_offset > _startOffset)
	{
		_buffer->setData(_localData.element_ptr(_startOffset), _startOffset, _offset - _startOffset);
		_startOffset = _offset;
	}
}

void ConstBuffer::reset()
{
	_offset = 0;
	_startOffset = 0;
}

}
