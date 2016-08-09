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
	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		ET_DECLARE_POINTER(VulkanIndexBuffer);
		
	public:
		VulkanIndexBuffer(IndexArray::Pointer, BufferDrawType, const std::string&);

		void bind() override;
        void clear() override;
	};
}
