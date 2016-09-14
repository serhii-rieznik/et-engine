/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/vertexbuffer.h>

namespace et
{
    struct MetalState;
    class MetalNativeBuffer;
	class MetalVertexBufferPrivate;
	class MetalVertexBuffer : public VertexBuffer
	{
	public:
		ET_DECLARE_POINTER(MetalVertexBuffer);
		
	public:
		MetalVertexBuffer(MetalState&, const VertexDeclaration&, const BinaryDataStorage&, BufferDrawType, const std::string&);
		~MetalVertexBuffer();
        
        const MetalNativeBuffer& nativeBuffer() const;

		void bind() override;

		void setData(const void* data, uint32_t dataSize, bool invalidateExistingData) override;
		void setDataWithOffset(const void* data, uint32_t offset, uint32_t dataSize) override;
		uint64_t dataSize() override;

		void* map(uint32_t offset, uint32_t dataSize, uint32_t options /* see MapBufferOptions */) override;
		bool mapped() const override;
		void unmap() override;

		void clear() override;

	private:
		ET_DECLARE_PIMPL(MetalVertexBuffer, 32)
	};
}
