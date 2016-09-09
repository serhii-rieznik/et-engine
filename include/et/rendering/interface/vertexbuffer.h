/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/vertexarray.h>
#include <et/rendering/base/vertexstorage.h>

namespace et
{
	class VertexBuffer : public Object
	{
	public:
		ET_DECLARE_POINTER(VertexBuffer);
		
	public:
		VertexBuffer(const VertexDeclaration& decl, BufferDrawType drawType, const std::string& name)
			: Object(name), _decl(decl), _drawType(drawType) { }
		~VertexBuffer() = default;

		const VertexDeclaration& declaration() const
			{ return _decl; }

		BufferDrawType drawType() const
			{ return _drawType; }

		const std::string& sourceObjectName() const
			{ return _sourceObjectName; }

		void setSourceObjectName(const std::string& tag)
			{ _sourceObjectName = tag; }

		virtual void bind() = 0;

		virtual void setData(const void* data, uint32_t dataSize, bool invalidateExistingData) = 0;
		virtual void setDataWithOffset(const void* data, uint32_t offset, uint32_t dataSize) = 0;

		virtual uint64_t dataSize() = 0;

		virtual void* map(uint32_t offset, uint32_t dataSize, uint32_t options /* see MapBufferOptions */) = 0;
		virtual bool mapped() const = 0;
		virtual void unmap() = 0;

		virtual void clear() = 0;

	private:
		VertexDeclaration _decl;
		std::string _sourceObjectName;
        BufferDrawType _drawType = BufferDrawType::Static;
	};
}
