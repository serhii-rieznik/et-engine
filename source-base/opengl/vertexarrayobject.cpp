/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/rendering/rendercontext.h>

using namespace et;

VertexArrayObject::VertexArrayObject(RenderContext* rc, VertexBuffer::Pointer vb, 
	IndexBuffer::Pointer ib, const std::string& aName) : APIObject(aName), _rc(rc), _vb(vb), _ib(ib)
{
	init();
}

VertexArrayObject::VertexArrayObject(RenderContext* rc, const std::string& aName) :
	APIObject(aName), _rc(rc)
{
	init();
}

VertexArrayObject::~VertexArrayObject()
{
	uint32_t buffer = apiHandle();
	if ((buffer != 0) && OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		_rc->renderState().bindVertexArrayObject(buffer);
		_rc->renderState().vertexArrayDeleted(buffer);
		glDeleteVertexArrays(1, &buffer);
		checkOpenGLError("glDeleteVertexArrays");
	}
}

void VertexArrayObject::init()
{
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		GLuint buffer = 0;
		glGenVertexArrays(1, &buffer);
		checkOpenGLError("glGenVertexArrays in %s", name().c_str());
		
		setAPIHandle(buffer);
		_rc->renderState().bindVertexArrayObject(buffer);
	}
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
}

void VertexArrayObject::setBuffers(VertexBuffer::Pointer vb, IndexBuffer::Pointer ib)
{
	_vb = vb;
	_ib = ib;
	
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArrayObject(apiHandle());
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
}

void VertexArrayObject::setVertexBuffer(VertexBuffer::Pointer vb)
{
	_vb = vb;
	
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArrayObject(apiHandle());
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
}

void VertexArrayObject::setIndexBuffer(IndexBuffer::Pointer ib)
{
	_ib = ib;
	
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArrayObject(apiHandle());
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
}
