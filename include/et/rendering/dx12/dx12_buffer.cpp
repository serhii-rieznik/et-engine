/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_buffer.h>

namespace et
{
class DX12BufferPrivate
{
public:

};

DX12Buffer::DX12Buffer(DX12& dx12, const Description & desc) 
{
	ET_PIMPL_INIT(DX12Buffer);
}

DX12Buffer::~DX12Buffer()
{
	ET_PIMPL_FINALIZE(DX12Buffer);
}

uint8_t* DX12Buffer::map(uint32_t begin, uint32_t length)
{
	return nullptr;
}

void DX12Buffer::modifyRange(uint64_t begin, uint64_t length)
{
}

void DX12Buffer::unmap()
{
}

bool DX12Buffer::mapped() const
{
	return false;
}

void DX12Buffer::updateData(uint32_t offset, const BinaryDataStorage &)
{
}

void DX12Buffer::transferData(Buffer::Pointer destination)
{
}

uint32_t DX12Buffer::size() const
{
	return uint32_t();
}

}

