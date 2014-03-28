/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
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
