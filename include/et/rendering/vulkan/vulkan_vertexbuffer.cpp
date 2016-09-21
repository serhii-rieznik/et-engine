/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_vertexbuffer.h>
#include <et/rendering/vulkan/vulkan.h>

namespace et
{

class VulkanVertexBufferPrivate
{
public:
	VulkanVertexBufferPrivate(VulkanState& v, uint32_t size) 
		: nativeBuffer(v, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, false)
		, dataSize(size)
	{ 
	}

	VulkanNativeBuffer nativeBuffer;
	uint32_t dataSize = 0;
};

VulkanVertexBuffer::VulkanVertexBuffer(VulkanState& vulkan, const VertexDeclaration& decl, const BinaryDataStorage& data, 
	BufferDrawType dt, const std::string& name) : VertexBuffer(decl, dt, name) 
{
	ET_PIMPL_INIT(VulkanVertexBuffer, vulkan, data.size());
	setData(data.data(), _private->dataSize, true);
}

VulkanVertexBuffer::~VulkanVertexBuffer()
{
	ET_PIMPL_FINALIZE(VulkanVertexBuffer);
}

void VulkanVertexBuffer::setData(const void * data, uint32_t dataSize, bool invalidateExistingData)
{
	setDataWithOffset(data, 0, dataSize);
}

void VulkanVertexBuffer::setDataWithOffset(const void * data, uint32_t offset, uint32_t dataSize)
{
	ET_ASSERT(offset + dataSize <= _private->dataSize);

	void* ptr = map(offset, dataSize, 0);
	memcpy(ptr, data, dataSize);
	unmap();
}

void VulkanVertexBuffer::clear()
{
}

uint64_t VulkanVertexBuffer::dataSize()
{
	return _private->dataSize;
}

void* VulkanVertexBuffer::map(uint32_t offset, uint32_t dataSize, uint32_t options /* see MapBufferOptions */)
{
	return _private->nativeBuffer.map(offset, dataSize);
}

bool VulkanVertexBuffer::mapped() const 
{
	return _private->nativeBuffer.mapped();
}

void VulkanVertexBuffer::unmap()
{
	_private->nativeBuffer.unmap();
}

const VulkanNativeBuffer& VulkanVertexBuffer::nativeBuffer() const
{
	return _private->nativeBuffer;
}


}
