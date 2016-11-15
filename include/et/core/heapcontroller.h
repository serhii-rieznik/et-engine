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
class HeapControllerPrivate;
class HeapController
{
public:
	HeapController();
	HeapController(uint32_t capacity, uint32_t granularity);
	~HeapController();

	uint32_t capacity() const;
	uint32_t requiredInfoSize() const;
	uint32_t currentlyAllocatedSize() const;

	void init(uint32_t capacity, uint32_t granularity);
	void setInfoStorage(void*);
	void setAutoCompress(bool);

	bool allocate(uint32_t size, uint32_t& offset);
	bool containsAllocationWithOffset(uint32_t offset);
	bool release(uint32_t offset);
	
	bool empty() const;
	void compress();
	void clear();

	void getAllocationIndexes(std::vector<uint32_t>&) const;

private:
	ET_DENY_COPY(HeapController);
	ET_DECLARE_PIMPL(HeapController, 128);
};

}
