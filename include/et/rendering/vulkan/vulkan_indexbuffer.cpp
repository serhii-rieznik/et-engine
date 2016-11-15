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
	VulkanIndexBufferPrivate(VulkanState& v, uint32_t size, uint32_t usage, bool cpuReadable)
		: nativeBuffer(v, size, usage, cpuReadable, true) { }

	VulkanNativeBuffer nativeBuffer;
};

VulkanIndexBuffer::VulkanIndexBuffer(VulkanState& vulkan, IndexArray::Pointer i,
	BufferDrawType dt, const std::string& name) : IndexBuffer(i, dt, name)
{
	bool cpuReadable = drawType() == BufferDrawType::Dynamic;
	uint32_t usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | (cpuReadable ? 0 : VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	ET_PIMPL_INIT(VulkanIndexBuffer, vulkan, i->dataSize(), usage, cpuReadable);

	if (drawType() == BufferDrawType::Dynamic)
	{
		void* ptr = _private->nativeBuffer.map(0, i->dataSize());
		memcpy(ptr, i->data(), i->dataSize());
		_private->nativeBuffer.unmap();
	}
	else
	{
		VulkanNativeBuffer stagingBuffer(vulkan, i->dataSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, true);
		void* ptr = stagingBuffer.map(0, i->dataSize());
		memcpy(ptr, i->data(), i->dataSize());
		stagingBuffer.unmap();

		_private->nativeBuffer.copyFrom(stagingBuffer);
	}
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
