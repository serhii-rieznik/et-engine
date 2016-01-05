/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/apiobject.h>
#include <et/vertexbuffer/vertexarray.h>
#include <et/vertexbuffer/vertexstorage.h>

namespace et
{
	class RenderState;
	
	class VertexBuffer : public APIObject
	{
	public:
		ET_DECLARE_POINTER(VertexBuffer)
		
	public:
		VertexBuffer(RenderContext*, const VertexDeclaration&, const BinaryDataStorage&, BufferDrawType,
			const std::string& = emptyString);
		
		VertexBuffer(RenderContext*, const VertexArray::Description&, BufferDrawType,
			const std::string& = emptyString);

		~VertexBuffer();
		
		size_t vertexCount() const
			{ return _dataSize / _decl.dataSize(); }
		
		const VertexDeclaration& declaration() const
			{ return _decl; }

		void setData(const void* data, size_t dataSize, bool invalidateExistingData = false);
		void setDataWithOffset(const void* data, size_t offset, size_t dataSize);
		
		void* map(size_t offset, size_t dataSize, uint32_t options /* see MapBufferOptions */);
		
		bool mapped() const
			{ return _mapped; }
		
		void unmap();

		void setSourceObjectName(const std::string& tag)
			{ _sourceObjectName = tag; }

		const std::string& sourceObjectName() const
			{ return _sourceObjectName; }

		void clear();
		
		size_t dataSize() const
			{ return _dataSize; }

	private:
		RenderContext* _rc = nullptr;
		VertexDeclaration _decl;
		AtomicBool _mapped;
		size_t _dataSize = 0;
		std::string _sourceObjectName;
		BufferDrawType _drawType = BufferDrawType::Static;
	};
}
