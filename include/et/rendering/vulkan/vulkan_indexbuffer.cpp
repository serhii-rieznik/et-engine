/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_indexbuffer.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanIndexBufferPrivate
{
public:
	VulkanIndexBufferPrivate(VulkanState& v, uint32_t size) 
		:  nativeBuffer(v, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
	{
	}

	VulkanNativeBuffer nativeBuffer;
};

VulkanIndexBuffer::VulkanIndexBuffer(VulkanState& vulkan, IndexArray::Pointer i, 
	BufferDrawType dt, const std::string& name) : IndexBuffer(i, dt, name)
{
	ET_PIMPL_INIT(VulkanIndexBuffer, vulkan, i->dataSize());

	void* ptr = _private->nativeBuffer.map(0, i->dataSize());
	memcpy(ptr, i->data(), i->dataSize());
	_private->nativeBuffer.unmap();
}

VulkanIndexBuffer::~VulkanIndexBuffer()
{
	ET_PIMPL_FINALIZE(VulkanIndexBuffer);
}

const VulkanNativeBuffer& VulkanIndexBuffer::nativeBuffer() const
{
	return _private->nativeBuffer;
}

}
