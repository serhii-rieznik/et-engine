/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	size_t memoryUsage();
	size_t availableMemory();
	
	void* allocateVirtualMemory(size_t);
	void deallocateVirtualMemory(void*, size_t);
}
