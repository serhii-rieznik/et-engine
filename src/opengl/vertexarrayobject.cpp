/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/rendering/rendercontext.h>

using namespace et;

VertexArrayObjectData::VertexArrayObjectData(RenderContext* rc, VertexBuffer::Pointer vb, 
	IndexBuffer::Pointer ib, const std::string& aName) : APIObject(aName), _rc(rc), _vb(vb), _ib(ib)
{
	init();
}

VertexArrayObjectData::VertexArrayObjectData(RenderContext* rc, const std::string& aName) :
	APIObject(aName), _rc(rc)
{
	init();
}

VertexArrayObjectData::~VertexArrayObjectData()
{
	uint32_t buffer = apiHandle();
	if ((buffer != 0) && OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		_rc->renderState().bindVertexArray(buffer);
		_rc->renderState().vertexArrayDeleted(buffer);
		glDeleteVertexArrays(1, &buffer);
		checkOpenGLError("glDeleteVertexArrays");
	}
}

void VertexArrayObjectData::init()
{
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		GLuint buffer = 0;
		glGenVertexArrays(1, &buffer);
		checkOpenGLError("glGenVertexArrays in %s", name().c_str());
		
		setAPIHandle(buffer);
		_rc->renderState().bindVertexArray(buffer);
	}
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
}

void VertexArrayObjectData::setBuffers(VertexBuffer::Pointer vb, IndexBuffer::Pointer ib)
{
	_vb = vb;
	_ib = ib;
	
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArray(apiHandle());
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
}

void VertexArrayObjectData::setVertexBuffer(VertexBuffer::Pointer vb)
{
	_vb = vb;
	
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArray(apiHandle());
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
}

void VertexArrayObjectData::setIndexBuffer(IndexBuffer::Pointer ib)
{
	_ib = ib;
	
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArray(apiHandle());
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
}
