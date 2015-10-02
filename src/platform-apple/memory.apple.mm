/*
* This file is part of `et engine`
* Copyright 2009-2015 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

#include <mach/mach.h>
#include <objc/runtime.h>
#include <et/core/memory.h>

using namespace et;

size_t et::memoryUsage()
{
	struct task_basic_info info = { };
	mach_msg_type_number_t size = sizeof(info);
	kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
	return (kerr == KERN_SUCCESS) ? info.resident_size : 0;
}

size_t et::availableMemory()
{
	mach_port_t host_port = mach_host_self();
	mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
	vm_size_t pagesize = 0;
	vm_statistics_data_t vm_stat = { };
	
	host_page_size(host_port, &pagesize);
	host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size);
	
	return vm_stat.free_count * pagesize;
}

void* et::allocateVirtualMemory(size_t size)
{
	vm_address_t* result = nullptr;
	vm_allocate(mach_task_self(), reinterpret_cast<vm_address_t*>(&result), size, VM_FLAGS_ANYWHERE);
	return result;
}

void et::deallocateVirtualMemory(void* ptr, size_t size)
{
	vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(ptr), size);
}
