/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/object.h>
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

		int tag;

	public:
		VertexArray();
		VertexArray(const VertexDeclaration& decl, size_t size);
		VertexArray(const VertexDeclaration& decl, int size);
		
		VertexDataChunk& smoothing()
			{ return _smoothing; }

		const VertexDataChunk& smoothing() const
			{ return _smoothing; }

		VertexDataChunk chunk(VertexAttributeUsage usage);
		Description generateDescription() const;
		
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
		size_t _size;
		VertexDeclaration _decl;
		VertexDataChunkList _chunks;
		VertexDataChunk _smoothing;
	};
	
	typedef std::vector<VertexArray::Pointer> VertexArrayList;
}
