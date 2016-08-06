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
    struct MetalState;
    class MetalNativeBuffer;
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
        
        const MetalNativeBuffer& nativeBuffer() const;

	private:
		ET_DECLARE_PIMPL(MetalIndexBuffer, 32)
	};
}
