/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
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
		enum MapBufferMode
		{
			MapBufferMode_ReadOnly,
			MapBufferMode_WriteOnly,
			MapBufferMode_ReadWrite,
			
			MapBufferMode_max
		};
	public:
		VertexBufferData(RenderContext* rc, const VertexArray::Description& desc,
			BufferDrawType vertexDrawType, const std::string& name = emptyString);

		~VertexBufferData();
	
		uint32_t glID() const
			{ return _vertexBuffer; }
		
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
		uint32_t _vertexBuffer;
		VertexDeclaration _decl;
		AtomicBool _mapped;
		size_t _dataSize = 0;
		size_t _sourceTag = 0;
		BufferDrawType _drawType = BufferDrawType_Static;
	};

}
