/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_vertexbuffer.h>

namespace et
{

DX12VertexBuffer::DX12VertexBuffer(const VertexDeclaration& decl, BufferDrawType dt, const std::string& name) : 
	VertexBuffer(decl, dt, name) 
{
}

void DX12VertexBuffer::bind()
{
}

void DX12VertexBuffer::setData(const void * data, size_t dataSize, bool invalidateExistingData)
{
}

void DX12VertexBuffer::setDataWithOffset(const void * data, size_t offset, size_t dataSize)
{
}

void DX12VertexBuffer::clear()
{
}

uint64_t DX12VertexBuffer::dataSize()
{
	return uint64_t();
}

void* DX12VertexBuffer::map(size_t offset, size_t dataSize, uint32_t options /* see MapBufferOptions */)
{
	return nullptr; 
}

bool DX12VertexBuffer::mapped() const 
{
	return false;
}

void DX12VertexBuffer::unmap()
{

}

}
