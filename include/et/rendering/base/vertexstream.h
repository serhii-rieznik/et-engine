/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/rendering.h>
#include <et/rendering/base/vertexdeclaration.h>
#include <et/rendering/interface/buffer.h>

namespace et
{
class VertexStream : public NamedObject
{
public:
	ET_DECLARE_POINTER(VertexStream);

public:
	VertexStream() = default;

	void setVertexBuffer(const Buffer::Pointer& vb, const VertexDeclaration& decl);
	void setIndexBuffer(const Buffer::Pointer& ib, IndexArrayFormat format, PrimitiveType pt);

	const VertexDeclaration& vertexDeclaration() const 
		{ return _vbDeclaration; }

	IndexArrayFormat indexArrayFormat() const 
		{ return _ibFormat; }

	PrimitiveType primitiveType() const 
		{ return _primitiveType; }

	Buffer::Pointer& vertexBuffer()
		{ return _vb; }
	Buffer::Pointer& indexBuffer()
		{ return _ib; }
	const Buffer::Pointer& vertexBuffer() const
		{ return _vb; }
	const Buffer::Pointer& indexBuffer() const
		{ return _ib; }

	uint32_t vertexCount() const;

private:
	Buffer::Pointer _vb;
	Buffer::Pointer _ib;
	VertexDeclaration _vbDeclaration;
	IndexArrayFormat _ibFormat = IndexArrayFormat::Count;
	PrimitiveType _primitiveType = PrimitiveType::Points;
};

inline void VertexStream::setVertexBuffer(const Buffer::Pointer& vb, const VertexDeclaration& decl)
{
	_vb = vb;
	_vbDeclaration = decl;
}

inline void VertexStream::setIndexBuffer(const Buffer::Pointer& ib, IndexArrayFormat format, PrimitiveType pt)
{
	_ib = ib;
	_ibFormat = format;
	_primitiveType = pt;
}

inline uint32_t VertexStream::vertexCount() const
{
	return _vb->size() / _vbDeclaration.sizeInBytes();
}

}
