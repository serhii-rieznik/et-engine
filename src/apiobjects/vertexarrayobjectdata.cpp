/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et/opengl/openglcaps.h>
#include <et/apiobjects/vertexarrayobjectdata.h>

using namespace et;

VertexArrayObjectData::VertexArrayObjectData(RenderContext* rc, VertexBuffer vb, IndexBuffer ib,
	const std::string& aName) : Object(aName), _rc(rc), _vb(vb), _ib(ib), _vao(0)
{
	init();
}

VertexArrayObjectData::VertexArrayObjectData(RenderContext* rc, const std::string& aName) :
	Object(aName), _rc(rc), _vao(0)
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
	_rc->renderState().vertexArrayDeleted(_vao);
}

void VertexArrayObjectData::init()
{
#if (ET_SUPPORT_VERTEX_ARRAY_OBJECTS)
	if (openGLCapabilites().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		glGenVertexArrays(1, &_vao);
		checkOpenGLError("glGenVertexArrays in %s", name().c_str());
		_rc->renderState().bindVertexArray(_vao);
	}
#endif
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
}

void VertexArrayObjectData::setBuffers(VertexBuffer vb, IndexBuffer ib)
{
	_vb = vb;
	_ib = ib;
	_rc->renderState().bindVertexArray(_vao);
	_rc->renderState().bindBuffers(_vb, _ib, true);
}

void VertexArrayObjectData::setVertexBuffer(VertexBuffer vb)
{
	_vb = vb;
	_rc->renderState().bindVertexArray(_vao);
	_rc->renderState().bindBuffers(_vb, _ib, true);
}

void VertexArrayObjectData::setIndexBuffer(IndexBuffer ib)
{
	_ib = ib;
	_rc->renderState().bindVertexArray(_vao);
	_rc->renderState().bindBuffers(_vb, _ib, true);
}
