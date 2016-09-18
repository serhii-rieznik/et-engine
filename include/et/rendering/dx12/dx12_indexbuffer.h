/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/indexbuffer.h>

namespace et
{
	class DX12IndexBuffer : public IndexBuffer
	{
	public:
		ET_DECLARE_POINTER(DX12IndexBuffer);
		
	public:
		DX12IndexBuffer(IndexArray::Pointer, BufferDrawType, const std::string&);
	};
}
