/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/apiobjects/vertexbuffer.h>
#include <et/apiobjects/indexbuffer.h>

namespace et
{
	class RenderState;
	class VertexArrayObjectData : public APIObject
	{
	public:
		VertexArrayObjectData(RenderContext*, VertexBuffer, IndexBuffer,
			const std::string& name = emptyString);
		
		VertexArrayObjectData(RenderContext*, const std::string& name = emptyString);
		
		~VertexArrayObjectData();

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
		RenderContext* _rc = nullptr;
		VertexBuffer _vb;
		IndexBuffer _ib;
	};
	
	typedef IntrusivePtr<VertexArrayObjectData> VertexArrayObject;
}
