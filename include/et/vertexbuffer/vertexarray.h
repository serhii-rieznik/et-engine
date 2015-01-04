/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/vertexbuffer/vertexdeclaration.h>
#include <et/vertexbuffer/vertexdatachunk.h>

namespace et
{ 
	class VertexArray : public Object
	{
	public:
		ET_DECLARE_POINTER(VertexArray)
		
		struct Description
		{
			VertexDeclaration declaration;
			BinaryDataStorage data;
		};

		int tag = 0;

	public:
		VertexArray();
		VertexArray(const VertexDeclaration& decl, size_t size);
		VertexArray(const VertexDeclaration& decl, int size);
		
		VertexDataChunk& smoothing()
			{ return _smoothing; }

		const VertexDataChunk& smoothing() const
			{ return _smoothing; }

		VertexDataChunk chunk(VertexAttributeUsage usage);
		
		const VertexDataChunk chunk(VertexAttributeUsage usage) const;
		
		const VertexDataChunkList& chunks() const
			{ return _chunks; }
		
		Description generateDescription() const;
		
		void resize(size_t size);
		void increase(size_t size);
		void fitToSize(size_t size);
		
		size_t size() const
			{ return _size; }

		const VertexDeclaration decl() const
			{ return _decl; }

		void serialize(std::ostream& stream);
		void deserialize(std::istream& stream);
		
		VertexArray* duplicate();
		
	private:
		size_t _size = 0;
		VertexDeclaration _decl;
		VertexDataChunkList _chunks;
		VertexDataChunk _smoothing;
	};
	
	typedef std::vector<VertexArray::Pointer> VertexArrayList;
}
