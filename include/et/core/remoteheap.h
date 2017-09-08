/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */
#pragma once

#if !defined(ET_CORE_INCLUDES)
#	error "Do not include this file directly, it is automatically included in et.h"
#endif

namespace et
{
struct RemoteHeapPrivate;
class RemoteHeap
{
public:
	RemoteHeap();
	RemoteHeap(uint32_t capacity, uint32_t granularity);
	RemoteHeap(RemoteHeap&&);
	RemoteHeap& operator = (RemoteHeap&&);
	~RemoteHeap();

	uint32_t capacity() const;
	uint32_t requiredInfoSize() const;
	uint32_t allocatedSize() const;

	void init(uint32_t capacity, uint32_t granularity);
	void setInfoStorage(void*);
	void clear();

	bool allocate(uint32_t size, uint32_t& offset);
	bool containsAllocationWithOffset(uint32_t offset);
	bool release(uint32_t offset);

	bool empty() const;

private:
	ET_DENY_COPY(RemoteHeap);
	ET_DECLARE_PIMPL(RemoteHeap, 128);
};

}
