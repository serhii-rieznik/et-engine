/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/indexbuffer.h>

namespace et
{

class VulkanState;
class VulkanNativeBuffer;
class VulkanIndexBufferPrivate;
class VulkanIndexBuffer : public IndexBuffer
{
public:
	ET_DECLARE_POINTER(VulkanIndexBuffer);
		
public:
	VulkanIndexBuffer(VulkanState& vulkan, IndexArray::Pointer, BufferDrawType, const std::string&);
	~VulkanIndexBuffer();

	const VulkanNativeBuffer& nativeBuffer() const;

private:
	ET_DECLARE_PIMPL(VulkanIndexBuffer, 128);
};

}
