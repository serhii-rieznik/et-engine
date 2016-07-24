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
	IndexBuffer::Pointer ib, const std::string& aName) : APIObject(aName), _vb(vb), _ib(ib)
{
	init();
}

VertexArrayObject::VertexArrayObject(RenderContext* rc, const std::string& aName) :
	APIObject(aName)
{
	init();
}

VertexArrayObject::~VertexArrayObject()
{
	uint32_t buffer = apiHandle();

	bool supported = OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects);
	if (supported && (buffer != 0) && glIsVertexArray(buffer))
	{
#	if (ET_EXPOSE_OLD_RENDER_STATE)
		_rc->renderState()->bindVertexArrayObject(buffer);
		_rc->renderState()->vertexArrayDeleted(buffer);
#	endif
		glDeleteVertexArrays(1, &buffer);
		checkOpenGLError("glDeleteVertexArrays");
	}
}

void VertexArrayObject::bind()
{
	if (OpenGLCapabilities::instance().hasFeature(OpenGLFeature_VertexArrayObjects))
	{
		glBindVertexArray(apiHandle());
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
	}

	rebind();
}

void VertexArrayObject::setBuffers(VertexBuffer::Pointer vb, IndexBuffer::Pointer ib)
{
	_vb = vb;
	_ib = ib;
	rebind();
}

void VertexArrayObject::setVertexBuffer(VertexBuffer::Pointer vb)
{
	_vb = vb;
	rebind();
}

void VertexArrayObject::setIndexBuffer(IndexBuffer::Pointer ib)
{
	_ib = ib;
	rebind();
}

void VertexArrayObject::rebind()
{
	bind();

	if (_vb.valid())
	{
		_vb->bind();
	}

	if (_ib.valid())
	{
		_ib->bind();
	}
}
