/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/indexbufferdata.h>

namespace et
{
	class IndexBuffer : public IntrusivePtr<IndexBufferData>
	{
	public:
		IndexBuffer() :
			IntrusivePtr<IndexBufferData>(nullptr) { }
		
	private:
		friend class VertexBufferFactory;
		
		IndexBuffer(IndexBufferData* data) :
			IntrusivePtr<IndexBufferData>(data) { }
	};

	typedef std::vector<IndexBuffer> IndexBufferList;
}
