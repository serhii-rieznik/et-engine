/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_vertexbuffer.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanVertexBufferPrivate
{
public:
	VulkanVertexBufferPrivate(VulkanState& v, uint32_t size, uint32_t usage, bool cpuReadable) 
		: vulkan(v), nativeBuffer(v, size, usage, cpuReadable, cpuReadable) , dataSize(size) { }

	VulkanState& vulkan;
	VulkanNativeBuffer nativeBuffer;
	uint32_t dataSize = 0;
};

VulkanVertexBuffer::VulkanVertexBuffer(VulkanState& vulkan, const VertexDeclaration& decl, const BinaryDataStorage& data, 
	BufferDrawType dt, const std::string& name) : VertexBuffer(decl, dt, name) 
{
	bool cpuReadable = drawType() == BufferDrawType::Dynamic;
	uint32_t usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | (cpuReadable ? 0 : VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	ET_PIMPL_INIT(VulkanVertexBuffer, vulkan, data.size(), usage, cpuReadable);

	setData(data.data(), 0, _private->dataSize);
}

VulkanVertexBuffer::~VulkanVertexBuffer()
{
	ET_PIMPL_FINALIZE(VulkanVertexBuffer);
}

void VulkanVertexBuffer::setData(const void * data, uint32_t offset, uint32_t dataSize)
{
	ET_ASSERT(offset + dataSize <= _private->dataSize);

	if (drawType() == BufferDrawType::Dynamic)
	{
		void* ptr = map(offset, dataSize, 0);
		memcpy(ptr, data, dataSize);
		unmap();
	}
	else
	{
		VulkanNativeBuffer stagingBuffer(_private->vulkan, _private->dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, true);
		void* ptr = stagingBuffer.map(offset, dataSize);
		memcpy(ptr, data, dataSize);
		stagingBuffer.unmap();
		_private->nativeBuffer.copyFrom(stagingBuffer);
	}
}

uint64_t VulkanVertexBuffer::dataSize()
{
	return _private->dataSize;
}

void* VulkanVertexBuffer::map(uint32_t offset, uint32_t dataSize, uint32_t options /* see MapBufferOptions */)
{
	return _private->nativeBuffer.map(offset, dataSize);
}

bool VulkanVertexBuffer::mapped() const 
{
	return _private->nativeBuffer.mapped();
}

void VulkanVertexBuffer::unmap()
{
	_private->nativeBuffer.unmap();
}

const VulkanNativeBuffer& VulkanVertexBuffer::nativeBuffer() const
{
	return _private->nativeBuffer;
}


}
