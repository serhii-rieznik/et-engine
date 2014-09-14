/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/opengl/opengl.h>

namespace et
{
	size_t vertexAttributeUsageMask(VertexAttributeUsage u);

	VertexAttributeUsage stringToVertexAttribute(const std::string& s);
	std::string vertexAttributeToString(VertexAttributeUsage va);

	size_t vertexAttributeTypeSize(VertexAttributeType t);
	size_t vertexAttributeTypeComponents(VertexAttributeType t);
	uint32_t vertexAttributeTypeDataType(VertexAttributeType t);
}
