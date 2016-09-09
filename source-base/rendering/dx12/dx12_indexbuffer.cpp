/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_indexbuffer.h>

namespace et
{

DX12IndexBuffer::DX12IndexBuffer(IndexArray::Pointer i, BufferDrawType dt, const std::string& name)
	: IndexBuffer(i, dt, name)
{
}

void DX12IndexBuffer::bind()
{
}

}
