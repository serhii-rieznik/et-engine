/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
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
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create VertexArrayObject in console application.")
#else
	init();
#endif
}

VertexArrayObjectData::VertexArrayObjectData(RenderContext* rc, const std::string& aName) :
	Object(aName), _rc(rc), _vao(0)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create VertexArrayObject in console application.")
#else
	init();
#endif
}

VertexArrayObjectData::~VertexArrayObjectData()
{
#if !defined(ET_CONSOLE_APPLICATION)
	
#	if (ET_SUPPORT_VERTEX_ARRAY_OBJECTS)
	if (_vao && openGLCapabilites().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		_rc->renderState().bindVertexArray(_vao);
		
		_rc->renderState().vertexArrayDeleted(_vao,
			(_vb.valid() ? _vb->glID() : 0), (_ib.valid() ? _ib->glID() : 0));
		
		glDeleteVertexArrays(1, &_vao);
		checkOpenGLError("glDeleteVertexArrays");
	}
#	endif
	
#endif
}

void VertexArrayObjectData::init()
{
#if !defined(ET_CONSOLE_APPLICATION)
	
#	if (ET_SUPPORT_VERTEX_ARRAY_OBJECTS)
	if (openGLCapabilites().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		glGenVertexArrays(1, &_vao);
		checkOpenGLError("glGenVertexArrays in %s", name().c_str());
		_rc->renderState().bindVertexArray(_vao);
	}
#	endif
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
	
#endif
}

void VertexArrayObjectData::setBuffers(VertexBuffer vb, IndexBuffer ib)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_vb = vb;
	_ib = ib;
	
	if (openGLCapabilites().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArray(_vao);
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
#endif
}

void VertexArrayObjectData::setVertexBuffer(VertexBuffer vb)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_vb = vb;
	
	if (openGLCapabilites().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArray(_vao);
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
#endif
}

void VertexArrayObjectData::setIndexBuffer(IndexBuffer ib)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_ib = ib;
	
	if (openGLCapabilites().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArray(_vao);
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
#endif
}
