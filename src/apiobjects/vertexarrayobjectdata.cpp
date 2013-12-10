/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/renderstate.h>
#include <et/opengl/openglcaps.h>
#include <et/apiobjects/vertexarrayobjectdata.h>

using namespace et;

VertexArrayObjectData::VertexArrayObjectData(RenderState& rs, VertexBuffer vb, IndexBuffer ib,
	const std::string& aName) : Object(aName), _rs(rs), _vb(vb), _ib(ib), _vao(0)
{
	init();
}

VertexArrayObjectData::VertexArrayObjectData(RenderState& rs, const std::string& aName) :
	Object(aName), _rs(rs), _vao(0)
{
	init();
}

VertexArrayObjectData::~VertexArrayObjectData()
{
#if (ET_SUPPORT_VERTEX_ARRAY_OBJECTS)
	if (openGLCapabilites().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		if (_vao && glIsVertexArray(_vao))
			glDeleteVertexArrays(1, &_vao);
	}
#endif
	_rs.vertexArrayDeleted(_vao);
}

void VertexArrayObjectData::init()
{
#if (ET_SUPPORT_VERTEX_ARRAY_OBJECTS)
	if (openGLCapabilites().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		glGenVertexArrays(1, &_vao);
		checkOpenGLError("glGenVertexArrays in %s", name().c_str());
		_rs.bindVertexArray(_vao);
	}
#endif
	
	_rs.bindBuffers(_vb, _ib, true);
}

void VertexArrayObjectData::setBuffers(VertexBuffer vb, IndexBuffer ib)
{
	_vb = vb;
	_ib = ib;
	_rs.bindVertexArray(_vao);
	_rs.bindBuffers(_vb, _ib, true);
}

void VertexArrayObjectData::setVertexBuffer(VertexBuffer vb)
{
	_vb = vb;
	_rs.bindVertexArray(_vao);
	_rs.bindBuffers(_vb, _ib, true);
}

void VertexArrayObjectData::setIndexBuffer(IndexBuffer ib)
{
	_ib = ib;
	_rs.bindVertexArray(_vao);
	_rs.bindBuffers(_vb, _ib, true);
}
