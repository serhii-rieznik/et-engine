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
		: vulkan(v) { }

	VulkanState& vulkan;
	Buffer::Description desc;
	Vector<Buffer::Range> modifiedRanges;
	std::atomic_bool mapped { false };
};

VulkanBuffer::VulkanBuffer(VulkanState& vulkan, const Description& desc)
{
	static Map<Usage, uint32_t> usageFlags = 
	{
		{ Usage::Constant, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT },
		{ Usage::Vertex, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT },
		{ Usage::Index, VK_BUFFER_USAGE_INDEX_BUFFER_BIT },
		{ Usage::Staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT },
	};

	ET_PIMPL_INIT(VulkanBuffer, vulkan);

	_private->desc.location = desc.location;
	_private->desc.usage = desc.usage;
	_private->desc.size = desc.size;
	_private->desc.alignedSize = alignUpTo(desc.size, uint32_t(_private->vulkan.physicalDeviceProperties.limits.nonCoherentAtomSize));

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

	VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.memoryTypeIndex = vulkan::getMemoryTypeIndex(_private->vulkan, _private->memoryRequirements.memoryTypeBits, memoryProperties);
	allocInfo.allocationSize = _private->memoryRequirements.size;
	VULKAN_CALL(vkAllocateMemory(_private->vulkan.device, &allocInfo, nullptr, &_private->memory));

	VULKAN_CALL(vkBindBufferMemory(_private->vulkan.device, _private->buffer, _private->memory, 0));

	if (desc.initialData.size() > 0)
		updateData(0, desc.initialData);
}

VulkanBuffer::~VulkanBuffer()
{
	vkDestroyBuffer(_private->vulkan.device, _private->buffer, nullptr);
	vkFreeMemory(_private->vulkan.device, _private->memory, nullptr);

	ET_PIMPL_FINALIZE(VulkanBuffer);
}

void VulkanBuffer::updateData(uint32_t offset, const BinaryDataStorage& data)
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
		Description desc;
		desc.initialData = BinaryDataStorage(data.data(), data.size());
		desc.location = Location::Host;
		desc.usage = Usage::Staging;
		desc.size = data.size();
		
		retain();
		VulkanBuffer stagingBuffer(_private->vulkan, desc);
		stagingBuffer.transferData(VulkanBuffer::Pointer(this));
		release();
	}
}

void VulkanBuffer::transferData(Buffer::Pointer dst)
{
	VulkanBuffer::Pointer destination = dst;
	ET_ASSERT(destination->_private->memoryRequirements.size >= _private->memoryRequirements.size);
	
	VkBufferCopy region = { 0, 0, _private->memoryRequirements.size };
	_private->vulkan.executeServiceCommands([&](VkCommandBuffer cmdBuffer)
	{
		vkCmdCopyBuffer(cmdBuffer, _private->buffer, destination->_private->buffer, 1, &region);
	});;
}

const VulkanNativeBuffer& VulkanBuffer::nativeBuffer() const
{
	return *(_private);
}

uint8_t* VulkanBuffer::map(uint32_t offset, uint32_t size)
{
	ET_ASSERT(size > 0);
	ET_ASSERT(_private->desc.location == Location::Host);
	ET_ASSERT(_private->mapped == false);

	size = alignUpTo(size, uint32_t(_private->vulkan.physicalDeviceProperties.limits.nonCoherentAtomSize));
	ET_ASSERT(offset + size <= _private->desc.alignedSize);

	void* pointer = nullptr;
	VULKAN_CALL(vkMapMemory(_private->vulkan.device, _private->memory, offset, size, 0, &pointer));
	_private->mapped = true;

	return reinterpret_cast<uint8_t*>(pointer);
}

void VulkanBuffer::modifyRange(uint64_t begin, uint64_t length)
{
	_private->modifiedRanges.emplace_back(begin, length);
}

void VulkanBuffer::unmap()
{
	ET_ASSERT(_private->mapped);
	ET_ASSERT(_private->modifiedRanges.size() > 0);

	Vector<VkMappedMemoryRange> vkRanges;
	vkRanges.reserve(_private->modifiedRanges.size());
	for (const auto& range : _private->modifiedRanges)
	{
		vkRanges.emplace_back();
		VkMappedMemoryRange& r = vkRanges.back();
		r.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		r.memory = _private->memory;
		r.offset = range.begin;
		r.size = alignUpTo(range.length, _private->vulkan.physicalDeviceProperties.limits.nonCoherentAtomSize);
	}

	VULKAN_CALL(vkFlushMappedMemoryRanges(_private->vulkan.device, static_cast<uint32_t>(vkRanges.size()), vkRanges.data()));
	vkUnmapMemory(_private->vulkan.device, _private->memory);

	_private->modifiedRanges.clear();
	_private->mapped = false;
}

bool VulkanBuffer::mapped() const
{
	return _private->mapped;
}

uint32_t VulkanBuffer::size() const
{
	return _private->desc.size;
}

}
