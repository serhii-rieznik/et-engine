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
	VulkanDataBufferPrivate(VulkanState& v, uint32_t size, uint32_t usage, bool readable)
		: vulkan(v), nativeBuffer(v, size, usage, readable, false), dataSize(size), cpuReadable(readable) { }

	VulkanState& vulkan;
	VulkanNativeBuffer nativeBuffer;
	uint32_t dataSize = 0;
	bool cpuReadable = false;
	bool mapped = false;
};

VulkanDataBuffer::VulkanDataBuffer(VulkanState& vulkan, uint32_t size) 
{
	uint32_t usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	ET_PIMPL_INIT(VulkanDataBuffer, vulkan, size, usage, true);
}

VulkanDataBuffer::VulkanDataBuffer(VulkanState& vulkan, const BinaryDataStorage& data) 
{
	uint32_t usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	ET_PIMPL_INIT(VulkanDataBuffer, vulkan, data.size(), usage, false);
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
		uint8_t* ptr = map(offset, dataSize);
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

uint32_t VulkanDataBuffer::size() const
{
	return _private->dataSize;
}

const VulkanNativeBuffer& VulkanDataBuffer::nativeBuffer() const
{
	return _private->nativeBuffer;
}

uint8_t* VulkanDataBuffer::map(uint32_t offset, uint32_t size)
{
	ET_ASSERT(_private->cpuReadable);
	ET_ASSERT(_private->mapped == false);
	
	_private->mapped = true;
	return reinterpret_cast<uint8_t*>(_private->nativeBuffer.map(offset, size));
}

void VulkanDataBuffer::unmap()
{
	ET_ASSERT(_private->mapped);
	_private->nativeBuffer.unmap();
	_private->mapped = false;
}

void VulkanDataBuffer::flushRanges(const Vector<Range>& ranges)
{
	Vector<VkMappedMemoryRange> vkRanges;
	vkRanges.reserve(ranges.size());
	for (const auto& range : ranges)
	{
		vkRanges.emplace_back();
		VkMappedMemoryRange& r = vkRanges.back();
		r.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		r.memory = _private->nativeBuffer.memory();
		r.offset = range.begin;
		r.size = range.length;
	}
	VULKAN_CALL(vkFlushMappedMemoryRanges(_private->vulkan.device, static_cast<uint32_t>(vkRanges.size()), vkRanges.data()));
}

}
