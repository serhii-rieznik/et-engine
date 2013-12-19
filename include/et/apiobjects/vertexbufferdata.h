/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/vertexbuffer/vertexarray.h>

namespace et
{
	class RenderState;
	
	class VertexBufferData : public Object
	{
	public:
		VertexBufferData(RenderContext* rc, const VertexArray::Description& desc,
			BufferDrawType vertexDrawType, const std::string& name = std::string());

		~VertexBufferData();
	
		uint32_t glID() const
			{ return _vertexBuffer; }
		
		size_t vertexCount() const
			{ return _dataSize / _decl.dataSize(); }
		
		const VertexDeclaration& declaration() const
			{ return _decl; }

		void setData(const void* data, size_t dataSize);

		void serialize(std::ostream& stream);
		void deserialize(std::istream& stream);

		void setSourceTag(size_t tag)
			{_sourceTag = tag; }

		size_t sourceTag() const
			{ return _sourceTag; }

	private:
		VertexBufferData(RenderContext* rc, const VertexDeclaration& decl, const void* vertexData,
			size_t vertexDataSize, BufferDrawType vertexDrawType, const std::string& name = std::string());

	private:
		RenderContext* _rc;
		uint32_t _vertexBuffer;
		VertexDeclaration _decl;
		size_t _dataSize;
		size_t _sourceTag;
		BufferDrawType _drawType;
	};

}