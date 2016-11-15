/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/databuffer.h>

namespace et
{
class VulkanState;
class VulkanNativeBuffer;
class VulkanDataBufferPrivate;
class VulkanDataBuffer : public DataBuffer
{
public:
	ET_DECLARE_POINTER(VulkanDataBuffer);
		
public:
	VulkanDataBuffer(VulkanState& vulkan, uint32_t size);
	VulkanDataBuffer(VulkanState& vulkan, const BinaryDataStorage& data);
	~VulkanDataBuffer();

	uint32_t size() const override;
	void setData(const void* data, uint32_t offset, uint32_t dataSize) override;
	
	uint8_t* map(uint32_t offset, uint32_t size) override;
	void unmap() override;

	void flushRanges(const Vector<Range>&) override;

	const VulkanNativeBuffer& nativeBuffer() const;

private:
	ET_DECLARE_PIMPL(VulkanDataBuffer, 128);
};
}
