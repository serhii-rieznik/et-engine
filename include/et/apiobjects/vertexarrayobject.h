/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/vertexarrayobjectdata.h>

namespace et
{
	class VertexArrayObject : public IntrusivePtr<VertexArrayObjectData>
	{
	public:
		VertexArrayObject() :
			IntrusivePtr<VertexArrayObjectData>() { }

	private:
		friend class VertexBufferFactory;

		VertexArrayObject(VertexArrayObjectData* data) :
			IntrusivePtr<VertexArrayObjectData>(data) { }
	};

	typedef std::vector<VertexArrayObject> VertexArrayObjectList;
}