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

VertexArrayObjectData::VertexArrayObjectData(RenderContext* rc, VertexBuffer::Pointer vb, IndexBuffer ib,
	const std::string& aName) : APIObject(aName), _rc(rc), _vb(vb), _ib(ib)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create VertexArrayObject in console application.")
#else
	init();
#endif
}

VertexArrayObjectData::VertexArrayObjectData(RenderContext* rc, const std::string& aName) :
	APIObject(aName), _rc(rc)
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
	uint32_t buffer = static_cast<uint32_t>(apiHandle());
	if ((buffer != 0) && OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		_rc->renderState().bindVertexArray(buffer);
		_rc->renderState().vertexArrayDeleted(buffer);
		glDeleteVertexArrays(1, &buffer);
		checkOpenGLError("glDeleteVertexArrays");
	}
#endif
}

void VertexArrayObjectData::init()
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		GLuint buffer = 0;
		glGenVertexArrays(1, &buffer);
		checkOpenGLError("glGenVertexArrays in %s", name().c_str());
		
		setAPIHandle(buffer);
		_rc->renderState().bindVertexArray(buffer);
	}
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
#endif
}

void VertexArrayObjectData::setBuffers(VertexBuffer::Pointer vb, IndexBuffer ib)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_vb = vb;
	_ib = ib;
	
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArray(static_cast<uint32_t>(apiHandle()));
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
#endif
}

void VertexArrayObjectData::setVertexBuffer(VertexBuffer::Pointer vb)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_vb = vb;
	
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArray(static_cast<uint32_t>(apiHandle()));
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
#endif
}

void VertexArrayObjectData::setIndexBuffer(IndexBuffer ib)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_ib = ib;
	
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
		_rc->renderState().bindVertexArray(static_cast<uint32_t>(apiHandle()));
	
	_rc->renderState().bindBuffers(_vb, _ib, true);
#endif
}
