/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_indexbuffer.h>

namespace et
{

VulkanIndexBuffer::VulkanIndexBuffer(IndexArray::Pointer i, BufferDrawType dt, const std::string& name)
	: IndexBuffer(i, dt, name)
{
}

void VulkanIndexBuffer::bind()
{
}

void VulkanIndexBuffer::clear()
{
}

}
