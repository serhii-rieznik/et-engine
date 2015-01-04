/*
* This file is part of `et engine`
* Copyright 2009-2015 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

#include <et/core/et.h>
#include <et/core/memory.h>

#if (ET_PLATFORM_WIN)

#include <Psapi.h>

using namespace et;

size_t et::memoryUsage()
{
	PROCESS_MEMORY_COUNTERS pmc = { sizeof(PROCESS_MEMORY_COUNTERS) };
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, pmc.cb);
	return pmc.WorkingSetSize;
}

size_t et::availableMemory()
{
	ET_FAIL("Not implemented.");
	return 0;
}

void* et::allocateVirtualMemory(size_t)
{
	ET_FAIL("Not implemented.");
	return nullptr;
}

void et::deallocateVirtualMemory(void*, size_t)
{
	ET_FAIL("Not implemented.");
}

#endif // ET_PLATFORM_WIN
