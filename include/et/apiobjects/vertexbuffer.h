/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/vertexbufferdata.h>

namespace et
{
	class VertexBuffer : public IntrusivePtr<VertexBufferData>
	{
	public:
		VertexBuffer() :
			IntrusivePtr<VertexBufferData>(nullptr) { }
		
	private:
		friend class VertexBufferFactory;

		VertexBuffer(VertexBufferData* data) :
			IntrusivePtr<VertexBufferData>(data) { }
	};

	typedef std::vector<VertexBuffer> VertexBufferList;
}
