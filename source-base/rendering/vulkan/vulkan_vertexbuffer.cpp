/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_vertexbuffer.h>

namespace et
{

VulkanVertexBuffer::VulkanVertexBuffer(const VertexDeclaration& decl, BufferDrawType dt, const std::string& name) : 
	VertexBuffer(decl, dt, name) 
{
}

void VulkanVertexBuffer::bind()
{
}

void VulkanVertexBuffer::setData(const void * data, size_t dataSize, bool invalidateExistingData)
{
}

void VulkanVertexBuffer::setDataWithOffset(const void * data, size_t offset, size_t dataSize)
{
}

void VulkanVertexBuffer::clear()
{
}

uint64_t VulkanVertexBuffer::dataSize()
{
	return uint64_t();
}

void* VulkanVertexBuffer::map(size_t offset, size_t dataSize, uint32_t options /* see MapBufferOptions */)
{
	return nullptr; 
}

bool VulkanVertexBuffer::mapped() const 
{
	return false;
}

void VulkanVertexBuffer::unmap()
{

}

}
