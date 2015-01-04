/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/apiobjects/apiobject.h>
#include <et/vertexbuffer/vertexarray.h>

namespace et
{
	class RenderState;
	
	class VertexBufferData : public APIObject
	{
	public:
		VertexBufferData(RenderContext* rc, const VertexArray::Description& desc,
			BufferDrawType vertexDrawType, const std::string& name = emptyString);

		~VertexBufferData();
		
		size_t vertexCount() const
			{ return _dataSize / _decl.dataSize(); }
		
		const VertexDeclaration& declaration() const
			{ return _decl; }

		void setData(const void* data, size_t dataSize);
		
		void* map(size_t offset, size_t dataSize, MapBufferMode mode);
		
		bool mapped() const
			{ return _mapped; }
		
		void unmap();

		void serialize(std::ostream& stream);
		void deserialize(std::istream& stream);

		void setSourceTag(size_t tag)
			{_sourceTag = tag; }

		size_t sourceTag() const
			{ return _sourceTag; }

	private:
		VertexBufferData(RenderContext* rc, const VertexDeclaration& decl, const void* vertexData,
			size_t vertexDataSize, BufferDrawType vertexDrawType, const std::string& name = emptyString);

	private:
		RenderContext* _rc = nullptr;
		VertexDeclaration _decl;
		AtomicBool _mapped;
		size_t _dataSize = 0;
		size_t _sourceTag = 0;
		BufferDrawType _drawType = BufferDrawType::Static;
	};

	typedef IntrusivePtr<VertexBufferData> VertexBuffer;
}
