/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/vertexbuffer.h>
#include <et/rendering/interface/indexbuffer.h>

namespace et
{
class VertexStream : public Object
{
public:
	ET_DECLARE_POINTER(VertexStream);

public:
	VertexStream() = default;

	VertexStream(VertexBuffer::Pointer vb, IndexBuffer::Pointer ib)
		{  setBuffers(vb, ib); }

	void setBuffers(VertexBuffer::Pointer vb, IndexBuffer::Pointer ib)
		{ _vb = vb; _ib = ib; }

	VertexBuffer::Pointer vertexBuffer() const 
		{ return _vb; }

	IndexBuffer::Pointer indexBuffer() const 
		{ return _ib; }

private:
	VertexBuffer::Pointer _vb;
	IndexBuffer::Pointer _ib;
};
}
