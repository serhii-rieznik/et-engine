/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/vulkan/vulkan_buffer.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanBufferPrivate : public VulkanNativeBuffer
{
public:
	VulkanBufferPrivate(VulkanState& v)
		: vulkan(v)
	{
	}

	VulkanState& vulkan;
	Buffer::Description desc;
	Vector<Buffer::Range> modifiedRanges;
	std::atomic_bool mapped{ false };
};

VulkanBuffer::VulkanBuffer(VulkanState& vulkan, const Description& desc)
{
	ET_PIMPL_INIT(VulkanBuffer, vulkan);

	static Map<Usage, uint32_t> usageFlags =
	{
		{ Usage::Constant, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT },
		{ Usage::Vertex, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT },
		{ Usage::Index, VK_BUFFER_USAGE_INDEX_BUFFER_BIT },
		{ Usage::Staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT },
	};

	_private->desc.location = desc.location;
	_private->desc.usage = desc.usage;
	_private->desc.size = desc.size;
	_private->desc.alignedSize = alignUpTo(desc.size, _private->vulkan.physicalDeviceProperties.limits.nonCoherentAtomSize);

	VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = _private->desc.alignedSize;
	createInfo.usage = usageFlags[desc.usage];
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.usage |= (desc.location == Location::Device) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;
	VULKAN_CALL(vkCreateBuffer(_private->vulkan.device, &createInfo, nullptr, &_private->buffer));

	vkGetBufferMemoryRequirements(_private->vulkan.device, _private->buffer, &_private->memoryRequirements);

	uint32_t memoryProperties = (desc.location == Location::Device) ?
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT :
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	uint32_t typeIndex = vulkan::getMemoryTypeIndex(_private->vulkan, _private->memoryRequirements.memoryTypeBits, memoryProperties);
	if (_private->vulkan.allocator.allocateSharedMemory(_private->memoryRequirements.size, typeIndex, _private->allocation))
	{
		VULKAN_CALL(vkBindBufferMemory(_private->vulkan.device, _private->buffer, _private->allocation.memory, _private->allocation.offset));
	}

	if (desc.initialData.size() > 0)
		updateData(0, desc.initialData);
}

VulkanBuffer::~VulkanBuffer()
{
	vkDestroyBuffer(_private->vulkan.device, _private->buffer, nullptr);
	_private->vulkan.allocator.release(_private->allocation);

	ET_PIMPL_FINALIZE(VulkanBuffer);
}

void VulkanBuffer::updateData(uint64_t offset, const BinaryDataStorage& data)
{
	ET_ASSERT(offset + data.size() <= _private->desc.alignedSize);

	if (_private->desc.location == Location::Host)
	{
		uint8_t* ptr = map(offset, data.size());
		memcpy(ptr, data.data(), data.size());
		modifyRange(offset, data.size());
		unmap();
	}
	else
	{
		const uint64_t StagingBufferPageSize = 4 * 1024 * 1024;
		uint64_t stagingBufferSize = std::min(data.size(), StagingBufferPageSize);

		Description desc;
		desc.location = Location::Host;
		desc.usage = Usage::Staging;
		desc.size = stagingBufferSize;
		VulkanBuffer stagingBuffer(_private->vulkan, desc);

		retain();
		uint64_t copyOffset = 0;
		uint64_t bytesRemaining = data.size();
		while (bytesRemaining > 0)
		{ 
			stagingBufferSize = std::min(stagingBufferSize, bytesRemaining);
			uint8_t* ptr = stagingBuffer.map(0, stagingBufferSize);
			memcpy(ptr, data.data() + copyOffset, stagingBufferSize);
			stagingBuffer.modifyRange(0, stagingBufferSize);
			stagingBuffer.unmap();

			stagingBuffer.transferData(VulkanBuffer::Pointer(this), 0, copyOffset, stagingBufferSize);
			bytesRemaining -= stagingBufferSize;
			copyOffset += stagingBufferSize;
		}
		release();
	}
}

void VulkanBuffer::transferData(Buffer::Pointer dst, uint64_t srcOffset, uint64_t dstOffset, uint64_t size)
{
	VulkanBuffer::Pointer destination = dst;
	ET_ASSERT(destination->_private->memoryRequirements.size >= size);

	uint64_t targetSize = std::min(size, destination->_private->memoryRequirements.size);
	VkBufferCopy region = { srcOffset, dstOffset, targetSize };
	_private->vulkan.executeServiceCommands(VulkanQueueClass::Graphics, [&](VkCommandBuffer cmdBuffer) {
		vkCmdCopyBuffer(cmdBuffer, _private->buffer, destination->_private->buffer, 1, &region);
	});;
}

const VulkanNativeBuffer& VulkanBuffer::nativeBuffer() const
{
	return *(_private);
}

uint8_t* VulkanBuffer::map(uint64_t offset, uint64_t size)
{
	ET_ASSERT(size > 0);
	ET_ASSERT(_private->desc.location == Location::Host);
	ET_ASSERT(_private->mapped == false);

	size = alignUpTo(size, _private->vulkan.physicalDeviceProperties.limits.nonCoherentAtomSize);
	ET_ASSERT(offset + size <= _private->desc.alignedSize);

	_private->mapped = true;
	return _private->vulkan.allocator.map(_private->allocation);
}

void VulkanBuffer::modifyRange(uint64_t begin, uint64_t length)
{
	_private->modifiedRanges.emplace_back(begin, length);
}

void VulkanBuffer::unmap()
{
	ET_ASSERT(_private->mapped);
	if (_private->modifiedRanges.size() > 0)
	{
		VkDeviceSize flushBegin = std::numeric_limits<VkDeviceSize>::max();
		VkDeviceSize flushEnd = 0;
		for (const auto& range : _private->modifiedRanges)
		{
			flushBegin = std::min(flushBegin, range.begin);
			flushEnd = std::max(flushEnd, range.begin + range.length);
		}
		ET_ASSERT(flushEnd > flushBegin);

		VkMappedMemoryRange flushRange = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		flushRange.memory = _private->allocation.memory;
		flushRange.offset = flushBegin;
		flushRange.size = alignUpTo(flushEnd - flushBegin, _private->vulkan.physicalDeviceProperties.limits.nonCoherentAtomSize);
		VULKAN_CALL(vkFlushMappedMemoryRanges(_private->vulkan.device, 1, &flushRange));
	}
	_private->vulkan.allocator.unmap(_private->allocation);

	_private->modifiedRanges.clear();
	_private->mapped = false;
}

bool VulkanBuffer::mapped() const
{
	return _private->mapped;
}

uint64_t VulkanBuffer::size() const
{
	return _private->desc.size;
}

}
