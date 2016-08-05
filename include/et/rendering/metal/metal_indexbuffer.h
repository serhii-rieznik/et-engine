/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/indexbuffer.h>
#include <et/rendering/metal/metal_buffer.h>

namespace et
{
    class MetalState;
	class MetalIndexBufferPrivate;
	class MetalIndexBuffer : public IndexBuffer
	{
	public:
		ET_DECLARE_POINTER(MetalIndexBuffer);
		
	public:
        MetalIndexBuffer(MetalState& metal, IndexArray::Pointer i, BufferDrawType drawType, const std::string& name);
		~MetalIndexBuffer();

		void bind() override;
        void clear() override;

	private:
		ET_DECLARE_PIMPL(MetalIndexBuffer, 32)
	};
}
