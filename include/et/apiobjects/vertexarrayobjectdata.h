/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/vertexbuffer.h>
#include <et/apiobjects/indexbuffer.h>

namespace et
{
	class RenderState;
	class VertexArrayObjectData : public Object
	{
	public:
		VertexArrayObjectData(RenderState& rs, VertexBuffer vb, IndexBuffer ib,
			const std::string& name = std::string());
		
		VertexArrayObjectData(RenderState& rs, const std::string& name = std::string());
		
		~VertexArrayObjectData();

		uint32_t glID() const
			{ return _vao; }

		VertexBuffer& vertexBuffer() 
			{ return _vb; };

		const VertexBuffer& vertexBuffer() const
			{ return _vb; };

		IndexBuffer& indexBuffer()
			{ return _ib; };

		const IndexBuffer& indexBuffer() const
			{ return _ib; };

		void setVertexBuffer(VertexBuffer ib);
		void setIndexBuffer(IndexBuffer ib);
		void setBuffers(VertexBuffer vb, IndexBuffer ib);

	private:
		void init();

	private:
		RenderState& _rs;
		VertexBuffer _vb;
		IndexBuffer _ib;
		uint32_t _vao;
	};
}