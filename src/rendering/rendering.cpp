/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendering.h>

namespace et
{

	const std::string VertexAttributeUsageNames[Usage_max] = 
	{ 
		"Vertex", "Normal", "Color", "Tangent", "Binormal",
		"TexCoord0", "TexCoord1", "TexCoord2", "TexCoord3"
	};

	const size_t VertexAttributeUsageMasks[Usage_max] = 
		{ 0x01, 0x02, 0x04, 0x08,	0x10, 0x20, 0x40, 0x80, 0x100, 0x200, };

	VertexAttributeUsage stringToVertexAttribute(const std::string& s)
	{
		for (VertexAttributeUsage i = Usage_Position; i < Usage_max; i = (VertexAttributeUsage)(i + 1))
		{
			if (s == VertexAttributeUsageNames[i])
				return i;
		}

		return Usage_Undefined;
	}

	std::string vertexAttributeToString(VertexAttributeUsage va)
		{ return (va < Usage_max) ? VertexAttributeUsageNames[va] : std::string(); }

	size_t vertexAttributeTypeComponents(VertexAttributeType t)
	{
		switch (t)
		{ 
		case Type_Float: 
		case Type_Int:
			return 1;

		case Type_Vec2: 
			return 2;

		case Type_Vec3: 
			return 3;

		case Type_Vec4: 
			return 4;

		default: 
			return 0;
		}
	}

	uint32_t vertexAttributeTypeDataType(VertexAttributeType t)
	{
		switch (t)
		{ 
		case Type_Float:
		case Type_Vec2: 
		case Type_Vec3: 
		case Type_Vec4: 
		case Type_Mat3: 
		case Type_Mat4: 
			return GL_FLOAT;

		case Type_Int:
			return GL_INT;

		default: 
			return GL_FLOAT;
		}
	}

	size_t vertexAttributeTypeSize(VertexAttributeType t)
	{
		static size_t floatSize = size_t(sizeof(float));
		static size_t intSize = size_t(sizeof(int));
		
		switch (t)
		{ 
		case Type_Float: 
			return floatSize;

		case Type_Vec2: 
			return 2 * floatSize;

		case Type_Vec3: 
			return 3 * floatSize;

		case Type_Vec4: 
			return 4 * floatSize;

		case Type_Mat3:   
			return 9 * floatSize;

		case Type_Mat4: 
			return 16 * floatSize;

		case Type_Int:
			return intSize;

		default: 
			return 0;
		}
	}

	size_t vertexAttributeUsageMask(VertexAttributeUsage u)
	{
		return VertexAttributeUsageMasks[u];
	}
}