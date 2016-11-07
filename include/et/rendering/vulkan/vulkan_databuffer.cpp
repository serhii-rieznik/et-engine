/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/vulkan/vulkan_databuffer.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanDataBufferPrivate
{
public:
	VulkanDataBufferPrivate(VulkanState& v, uint32_t size, bool readable) 
		: vulkan(v)
		, nativeBuffer(v, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | (readable ? 0 : VK_BUFFER_USAGE_TRANSFER_DST_BIT), readable)
		, dataSize(size)
		, cpuReadable(readable)
	{ 
	}

	VulkanState& vulkan;
	VulkanNativeBuffer nativeBuffer;
	uint32_t dataSize = 0;
	bool cpuReadable = false;
};

VulkanDataBuffer::VulkanDataBuffer(VulkanState& vulkan, uint32_t size) 
{
	ET_PIMPL_INIT(VulkanDataBuffer, vulkan,size, true);
}

VulkanDataBuffer::VulkanDataBuffer(VulkanState& vulkan, const BinaryDataStorage& data) 
{
	ET_PIMPL_INIT(VulkanDataBuffer, vulkan, data.size(), false);
	setData(data.data(), 0, _private->dataSize);
}


VulkanDataBuffer::~VulkanDataBuffer()
{
	ET_PIMPL_FINALIZE(VulkanDataBuffer);
}

void VulkanDataBuffer::setData(const void* data, uint32_t offset, uint32_t dataSize)
{
	ET_ASSERT(offset + dataSize <= _private->dataSize);

	if (_private->cpuReadable)
	{
		void* ptr = _private->nativeBuffer.map(offset, dataSize);
		memcpy(ptr, data, dataSize);
		_private->nativeBuffer.unmap();
	}
	else
	{
		VulkanNativeBuffer stagingBuffer(_private->vulkan, _private->dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
		void* ptr = stagingBuffer.map(offset, dataSize);
		memcpy(ptr, data, dataSize);
		stagingBuffer.unmap();
		
		_private->nativeBuffer.copyFrom(stagingBuffer);
	}
}

uint32_t VulkanDataBuffer::size() const
{
	return _private->dataSize;
}

const VulkanNativeBuffer& VulkanDataBuffer::nativeBuffer() const
{
	return _private->nativeBuffer;
}


}
