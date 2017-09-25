/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan.h>

namespace et
{
class VulkanMemoryAllocatorPrivate
{
public:
	enum : uint64_t
	{
		PageSize = 64 * 1024 * 1024,
		PageIndexShift = 56,
		Alignment = 1024,
	};
	
	struct MemoryPage
	{
		uint32_t type = 0;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		RemoteHeap controller;
		void* info = nullptr;
		bool exclusive = false;
		bool unused = false;

		MemoryPage(uint32_t typ, uint32_t cap, VulkanState& st) :
			type(typ), controller(cap, static_cast<uint32_t>(Alignment))
		{
			uint32_t infoSize = controller.requiredInfoSize();
			info =  sharedBlockAllocator().allocate(infoSize);
			controller.setInfoStorage(info);
		}

		MemoryPage(MemoryPage&& r) : 
			type(r.type), memory(r.memory), controller(std::move(r.controller)), info(r.info), 
			exclusive(r.exclusive), unused(r.unused)
		{
			r.info = nullptr;
			r.memory = VK_NULL_HANDLE;
			r.type = 0;
			r.exclusive = false;
			r.unused = true;
		}

		MemoryPage& operator = (MemoryPage&& r)
		{
			type = r.type;
			memory = r.memory;
			info = r.info;
			controller = std::move(controller);
			exclusive = r.exclusive;
			unused = r.unused;

			r.info = nullptr;
			r.memory = VK_NULL_HANDLE;
			r.type = 0;
			r.unused = true;
			return *this;
		}
		
		~MemoryPage()
		{
			if (info != nullptr)
				sharedBlockAllocator().release(info);
		}
	};

	VulkanState& state;
	std::atomic_uint64_t allocationId { 0 };
	Set<VulkanMemoryAllocator::Allocation> allocations;
	Vector<MemoryPage> pages;
	
	VulkanMemoryAllocatorPrivate(VulkanState& st) :
		state(st) 
	{
		pages.reserve(256);
	}
};

VulkanMemoryAllocator::~VulkanMemoryAllocator()
{
	for (VulkanMemoryAllocatorPrivate::MemoryPage& page : _private->pages)
		vkFreeMemory(_private->state.device, page.memory, nullptr);

	ET_PIMPL_FINALIZE(VulkanMemoryAllocator);
}

void VulkanMemoryAllocator::init(VulkanState& state)
{
	ET_PIMPL_INIT(VulkanMemoryAllocator, state);
}

bool VulkanMemoryAllocator::allocateSharedMemory(VkDeviceSize size, uint32_t type, Allocation& alloc)
{
	uint32_t requestedSize = static_cast<uint32_t>(size);

	uint64_t id = ++_private->allocationId;
	uint32_t offset = 0;
	VkDeviceMemory memory = VK_NULL_HANDLE;

	bool allocated = false;
	uint64_t pageIndex = 0;
	for (VulkanMemoryAllocatorPrivate::MemoryPage& page : _private->pages)
	{
		if (!page.unused && !page.exclusive && (page.type == type) && page.controller.allocate(requestedSize, offset))
		{
			id ^= pageIndex << VulkanMemoryAllocatorPrivate::PageIndexShift;
			memory = page.memory;
			allocated = true;
			break;
		}
		++pageIndex;
	}

	if (allocated == false)
	{
		uint32_t alignedSize = alignUpTo(requestedSize, static_cast<uint32_t>(VulkanMemoryAllocatorPrivate::Alignment));
		uint32_t pageSize = std::max(alignedSize, static_cast<uint32_t>(VulkanMemoryAllocatorPrivate::PageSize));

		id ^= static_cast<uint64_t>(_private->pages.size()) << VulkanMemoryAllocatorPrivate::PageIndexShift;
		_private->pages.emplace_back(type, pageSize, _private->state);
		VulkanMemoryAllocatorPrivate::MemoryPage& page = _private->pages.back();

		VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocInfo.allocationSize = pageSize;
		allocInfo.memoryTypeIndex = type;
		VULKAN_CALL(vkAllocateMemory(_private->state.device, &allocInfo, nullptr, &page.memory));
		
		memory = page.memory;
		allocated = page.controller.allocate(requestedSize, offset);
	}

	if (allocated)
	{
		alloc.id = id;
		alloc.memory = memory;
		alloc.offset = offset;
		_private->allocations.insert(alloc);
	}

	return allocated;
}

bool VulkanMemoryAllocator::allocateExclusiveMemory(VkDeviceSize size, uint32_t type, Allocation& alloc)
{
	uint32_t requestedSize = static_cast<uint32_t>(size);

	uint64_t id = ++_private->allocationId;
	uint32_t offset = 0;

	uint32_t alignedSize = alignUpTo(requestedSize, static_cast<uint32_t>(VulkanMemoryAllocatorPrivate::Alignment));

	id ^= static_cast<uint64_t>(_private->pages.size()) << VulkanMemoryAllocatorPrivate::PageIndexShift;
	_private->pages.emplace_back(type, alignedSize, _private->state);
	VulkanMemoryAllocatorPrivate::MemoryPage& page = _private->pages.back();
	page.exclusive = true;

	VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = alignedSize;
	allocInfo.memoryTypeIndex = type;
	VULKAN_CALL(vkAllocateMemory(_private->state.device, &allocInfo, nullptr, &page.memory));

	bool allocated = page.controller.allocate(requestedSize, offset);

	if (allocated)
	{
		alloc.id = id;
		alloc.memory = page.memory;
		alloc.offset = offset;
		_private->allocations.insert(alloc);
	}

	return allocated;
}

bool VulkanMemoryAllocator::release(const Allocation& alloc)
{
	auto i = _private->allocations.find(alloc);
	ET_ASSERT(i != _private->allocations.end());

	uint64_t pageIndex = alloc.id >> VulkanMemoryAllocatorPrivate::PageIndexShift;
	ET_ASSERT(pageIndex < _private->pages.size());

	VulkanMemoryAllocatorPrivate::MemoryPage& page = _private->pages[pageIndex];
	bool released = page.controller.release(static_cast<uint32_t>(alloc.offset));

	if (page.controller.empty())
	{
		vkFreeMemory(_private->state.device, page.memory, nullptr);
		sharedBlockAllocator().release(page.info);
		page.memory = VK_NULL_HANDLE;
		page.info = nullptr;
		page.unused = true;
	}

	_private->allocations.erase(i);

	return released;
}

uint8_t* VulkanMemoryAllocator::map(const Allocation& alloc)
{
	auto i = _private->allocations.find(alloc);
	ET_ASSERT(i != _private->allocations.end());
	VulkanMemoryAllocator::Allocation allocation = *i;

	if (allocation.mapCounter == 0)
	{
		uint64_t pageIndex = allocation.id >> VulkanMemoryAllocatorPrivate::PageIndexShift;
		ET_ASSERT(pageIndex < _private->pages.size());
		const VulkanMemoryAllocatorPrivate::MemoryPage& page = _private->pages[pageIndex];

		void* mappedPtr = nullptr;
		VULKAN_CALL(vkMapMemory(_private->state.device, page.memory, 0, page.controller.capacity(), 0, &mappedPtr));
		allocation.mappedData = reinterpret_cast<uint8_t*>(mappedPtr);
	}
	allocation.mapCounter++;
	
	_private->allocations.erase(allocation);
	_private->allocations.insert(allocation);
	
	return allocation.mappedData + alloc.offset;
}

void VulkanMemoryAllocator::unmap(const Allocation& alloc)
{
	auto i = _private->allocations.find(alloc);
	ET_ASSERT(i != _private->allocations.end());
	VulkanMemoryAllocator::Allocation allocation = *i;

	if (allocation.mapCounter == 1)
	{
		uint64_t pageIndex = i->id >> VulkanMemoryAllocatorPrivate::PageIndexShift;
		ET_ASSERT(pageIndex < _private->pages.size());
		const VulkanMemoryAllocatorPrivate::MemoryPage& page = _private->pages[pageIndex];
		
		vkUnmapMemory(_private->state.device, page.memory);
		allocation.mappedData = nullptr;
	}
	ET_ASSERT(allocation.mapCounter > 0);
	--allocation.mapCounter;
	
	_private->allocations.erase(allocation);
	_private->allocations.insert(allocation);
}

}
