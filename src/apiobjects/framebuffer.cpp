/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/apiobjects/framebuffer.h>
#include <et/rendering/rendercontext.h>
#include <et/opengl/openglcaps.h>

using namespace et;

extern std::string FramebufferStatusToString(uint32_t status);
extern const uint32_t colorAttachments[Framebuffer::MaxRenderTargets];

Framebuffer::Framebuffer(RenderContext* rc, const FramebufferDescription& desc,
	const std::string& aName) : Object(aName), _rc(rc), _description(desc),
	_id(0), _numTargets(0), _colorRenderbuffer(0), _depthRenderbuffer(0)
{
	checkOpenGLError("Framebuffer::Framebuffer %s", name().c_str());

	glGenFramebuffers(1, &_id);
	checkOpenGLError("Framebuffer::Framebuffer -> glGenFramebuffers");

	_rc->renderState().bindFramebuffer(_id);

	bool hasColor = (_description.colorFormat != 0) && (_description.colorInternalformat != 0) &&
		(_description.colorType != 0) && (_description.numColorRenderTargets > 0);
	
	bool hasDepth = (_description.depthFormat != 0) && (_description.depthInternalformat != 0) &&
		(_description.depthType != 0);
	
	if (hasColor)
	{
		if (_description.colorIsRenderbuffer)
		{
			createOrUpdateColorRenderbuffer();
		}
		else 
		{
			for (size_t i = 0; i < _description.numColorRenderTargets; ++i)
			{ 
				Texture c;
				if (_description.isCubemap)
				{
					c = _rc->textureFactory().genCubeTexture(_description.colorInternalformat, _description.size.x,
						_description.colorFormat, _description.colorType, name() + "_color_" + intToStr(i));
				}
				else 
				{
					size_t dataSize = _description.size.square() *
						bitsPerPixelForTextureFormat(_description.colorInternalformat, _description.colorType) / 8;
					
					c = _rc->textureFactory().genTexture(GL_TEXTURE_2D, _description.colorInternalformat,
						_description.size, _description.colorFormat, _description.colorType,
						BinaryDataStorage(dataSize, 0), name() + "_color_" + intToStr(i));
				}
				c->setWrap(rc, TextureWrap_ClampToEdge, TextureWrap_ClampToEdge);
				addRenderTarget(c);
			}
		}
	}

	if (hasDepth)
	{
		if (_description.depthIsRenderbuffer)
		{
			createOrUpdateDepthRenderbuffer();
		}
		else 
		{
			Texture d;
			if (_description.isCubemap && (openGLCapabilites().version() == OpenGLVersion_New))
			{
				d = _rc->textureFactory().genCubeTexture(_description.depthInternalformat, _description.size.x,
					_description.depthFormat, _description.depthType, name() + "_depth");
			}
			else 
			{
				size_t dataSize = _description.size.square() *
					bitsPerPixelForTextureFormat(_description.depthInternalformat, _description.depthType) / 8;
				
				d = _rc->textureFactory().genTexture(GL_TEXTURE_2D, _description.depthInternalformat, _description.size,
					_description.depthFormat, _description.depthType, BinaryDataStorage(dataSize, 0), name() + "_depth");
			}
			
			if (d.valid())
			{
				d->setWrap(rc, TextureWrap_ClampToEdge, TextureWrap_ClampToEdge);
				setDepthTarget(d);
			}
		}
	}

#if (!ET_OPENGLES)
	if (!hasColor)
	{
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);
	}
#endif
	
	if (hasColor || hasDepth)
		checkStatus();
}

Framebuffer::Framebuffer(RenderContext* rc, uint32_t fboId, const std::string& aName) :
	Object(aName), _rc(rc), _id(fboId), _numTargets(0), _colorRenderbuffer(0), _depthRenderbuffer(0)
{
	if (glIsFramebuffer(fboId))
	{
		rc->renderState().bindFramebuffer(fboId);
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &_description.size.x);
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_description.size.y);
	}
	else if (fboId == 0)
	{
		_description.size = rc->sizei();
	}
}

Framebuffer::~Framebuffer()
{
	if (_colorRenderbuffer && glIsRenderbuffer(_colorRenderbuffer))
	{
		glDeleteRenderbuffers(1, &_colorRenderbuffer);
		checkOpenGLError("glDeleteRenderbuffers");
	}
	
	if (_depthRenderbuffer && glIsRenderbuffer(_depthRenderbuffer))
	{
		glDeleteRenderbuffers(1, &_depthRenderbuffer);
		checkOpenGLError("glDeleteRenderbuffers");
	}
	
	if (glIsFramebuffer(_id))
	{
		glDeleteFramebuffers(1, &_id);
		checkOpenGLError("glDeleteFramebuffers");
	}

	_rc->renderState().frameBufferDeleted(_id);
}

bool Framebuffer::checkStatus()
{
#if (ET_DEBUG)
	_rc->renderState().bindFramebuffer(_id);
	uint32_t status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		log::error("%s for %s", FramebufferStatusToString(status).c_str(), name().c_str());
	return status == GL_FRAMEBUFFER_COMPLETE;
#else
	return true;
#endif
}

bool Framebuffer::addRenderTarget(const Texture& rt)
{
	if (!rt.valid() || (rt->size() != _description.size)) return false;
	assert(glIsTexture(rt->glID()));

	_rc->renderState().bindFramebuffer(_id);

	if (openGLCapabilites().version() == OpenGLVersion_New)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, colorAttachments[_numTargets], rt->glID(), 0);
		checkOpenGLError("glFramebufferTexture(...) - %s", name().c_str());
	}
	else
	{
		if (rt->target() == GL_TEXTURE_2D)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[_numTargets], GL_TEXTURE_2D, rt->glID(), 0);
			checkOpenGLError("glFramebufferTexture2D(...) - %s", name().c_str());
		}
		else if (rt->target() == GL_TEXTURE_CUBE_MAP)
		{
			for (GLenum i = 0; i < 6; ++i)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[_numTargets], 
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, rt->glID(), 0);
			}
			checkOpenGLError("glFramebufferTexture2D(...) - %s", name().c_str());
		}
	}

	_renderTargets[_numTargets++] = rt;

	return checkStatus();
}

void Framebuffer::setDepthTarget(const Texture& rt)
{
	if (!rt.valid() || (rt->size() != _description.size)) return;

	_depthBuffer = rt;
	_rc->renderState().bindFramebuffer(_id);

	if (openGLCapabilites().version() == OpenGLVersion_New)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rt->glID(), 0);
		checkOpenGLError("glFramebufferTexture");
	}
	else
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->glID(), 0);
		checkOpenGLError("glFramebufferTexture2D");
	}

}

void Framebuffer::setDepthTarget(const Texture& texture, uint32_t target)
{
	if (!texture.valid() || (texture->size() != _description.size)) return;
	
	_rc->renderState().bindFramebuffer(_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, texture->glID(), 0);
	checkOpenGLError("glFramebufferTexture2D(...) - %s", name().c_str());
}

void Framebuffer::addSameRendertarget()
{
	if (_numTargets == 0) return;

	Texture& prev = _renderTargets[_numTargets - 1];

	std::string texName = name() + "_color_" + intToStr(_numTargets);
	
	Texture c;
	if (_description.isCubemap)
	{
		c = _rc->textureFactory().genCubeTexture(prev->internalFormat(), prev->width(),
			prev->format(), prev->dataType(), texName);
	}
	else
	{
		BinaryDataStorage emptyData(prev->size().square() *
			bitsPerPixelForTextureFormat(prev->internalFormat(), prev->dataType()) / 8, 0);
		
		c = _rc->textureFactory().genTexture(prev->target(), prev->internalFormat(),
			prev->size(), prev->format(), prev->dataType(), emptyData, texName);
	}
	
	c->setWrap(_rc, TextureWrap_ClampToEdge, TextureWrap_ClampToEdge);
	addRenderTarget(c);
}

void Framebuffer::setCurrentRenderTarget(const Texture& texture)
{
	assert(texture.valid());
	
	_rc->renderState().bindFramebuffer(_id);
	if (openGLCapabilites().version() == OpenGLVersion_New)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, colorAttachments[0], texture->glID(), 0);
		checkOpenGLError("glFramebufferTexture");
	}
	else
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[0], texture->target(), texture->glID(), 0);
		checkOpenGLError("glFramebufferTexture2D");
	}
}

void Framebuffer::setCurrentRenderTarget(const Texture& texture, uint32_t target)
{
	assert(texture.valid());
	_rc->renderState().bindFramebuffer(_id);

	if (openGLCapabilites().version() == OpenGLVersion_New)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, colorAttachments[0], texture->glID(), 0);
		checkOpenGLError("glFramebufferTexture");
	}
	else
	{
		if (target == GL_TEXTURE_CUBE_MAP)
		{
			for (GLenum i = 0; i < 6; ++i)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[_numTargets],
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, texture->glID(), 0);
				checkOpenGLError("glFramebufferTexture2D");
			}
		}
		else
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[_numTargets], target, texture->glID(), 0);
			checkOpenGLError("glFramebufferTexture2D");
		}
	}
}

void Framebuffer::setCurrentRenderTarget(size_t index)
{
	assert(index < _numTargets);
	assert(_renderTargets[index].valid());
	setCurrentRenderTarget(_renderTargets[index]);
}

#if (ET_OPENGLES)

void Framebuffer::setDrawBuffersCount(int)
{
	assert(false && "glDrawBuffers is not supported in OpenGL ES");
}

#else

void Framebuffer::setDrawBuffersCount(int count)
{
	_rc->renderState().bindFramebuffer(_id);
	glDrawBuffers(count, colorAttachments);
	checkOpenGLError("Framebuffer::setDrawBuffersCount -> glDrawBuffers - %s", name().c_str());
	checkStatus();
}

#endif

void Framebuffer::setCurrentCubemapFace(uint32_t faceIndex)
{
	assert(_description.isCubemap);
	
	_rc->renderState().bindFramebuffer(_id);

	uint32_t target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex;
	
	if (_renderTargets[0].valid())
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[0], target, _renderTargets[0]->glID(), 0);
		checkOpenGLError("setCurrentCubemapFace -> color");
	}
	
	if (_depthBuffer.valid())
	{
		if (openGLCapabilites().version() == OpenGLVersion_Old)
			target = GL_TEXTURE_2D;
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, _depthBuffer->glID(), 0);
		checkOpenGLError("setCurrentCubemapFace -> depth");
	}
}

void Framebuffer::createOrUpdateColorRenderbuffer()
{
	if (!glIsRenderbuffer(_colorRenderbuffer))
	{
		glGenRenderbuffers(1, &_colorRenderbuffer);
		checkOpenGLError("glGenRenderbuffers");
	}
	
	_rc->renderState().bindFramebuffer(_id);
	_rc->renderState().bindRenderbuffer(_colorRenderbuffer);

#if (!ET_PLATFORM_ANDROID)
	if (_description.numSamples > 1)
	{
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, _description.numSamples,
			_description.colorInternalformat, _description.size.x, _description.size.y);
		checkOpenGLError("glRenderbufferStorageMultisample");
	}
	else
#endif
	{
		glRenderbufferStorage(GL_RENDERBUFFER, _description.colorInternalformat,
			_description.size.x, _description.size.y);
		checkOpenGLError("glRenderbufferStorage");
	}
	
	setColorRenderbuffer(_colorRenderbuffer);
}

void Framebuffer::createOrUpdateDepthRenderbuffer()
{
	if (!glIsRenderbuffer(_depthRenderbuffer))
	{
		glGenRenderbuffers(1, &_depthRenderbuffer);
		checkOpenGLError("glGenRenderbuffers");
	}
	
	_rc->renderState().bindFramebuffer(_id);
	_rc->renderState().bindRenderbuffer(_depthRenderbuffer);
	
#if (!ET_PLATFORM_ANDROID)
	if (_description.numSamples > 1)
	{
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, _description.numSamples,
			_description.depthInternalformat, _description.size.x, _description.size.y);
		checkOpenGLError("glRenderbufferStorageMultisample");
	}
	else
#endif
	{
		glRenderbufferStorage(GL_RENDERBUFFER, _description.depthInternalformat,
			_description.size.x, _description.size.y);
		checkOpenGLError("glRenderbufferStorage");
	}
	
	setDepthRenderbuffer(_depthRenderbuffer);
}

void Framebuffer::resize(const vec2i& sz)
{
	_description.size = sz;
	
	bool hasColor = (_description.colorFormat != 0) && (_description.colorInternalformat != 0) &&
		(_description.colorType != 0) && (_description.numColorRenderTargets > 0);
	
	bool hasDepth = (_description.depthFormat != 0) && (_description.depthInternalformat != 0) &&
		(_description.depthType != 0);

	if (hasColor)
	{
		if (_description.colorIsRenderbuffer)
		{
			createOrUpdateColorRenderbuffer();
		}
		else if (_numTargets > 0)
		{
			for (size_t i = 0; i < _numTargets; ++i)
			{
				TextureDescription::Pointer desc = _renderTargets[i]->description();
				desc->size = sz;
				desc->data.resize(desc->layersCount * desc->dataSizeForAllMipLevels());
				_renderTargets[i]->updateData(_rc, desc);
				setCurrentRenderTarget(_renderTargets[i], _renderTargets[i]->target());
			}
		}
	}
	
	if (hasDepth)
	{
		if (_description.depthIsRenderbuffer)
		{
			createOrUpdateDepthRenderbuffer();
		}
		else if (_depthBuffer.valid())
		{
			auto desc = _depthBuffer->description();
			desc->size = sz;
			desc->data.resize(desc->dataSizeForAllMipLevels());
			_depthBuffer->updateData(_rc, desc);
			setDepthTarget(_depthBuffer);
		}
	}
	
	if (hasColor || hasDepth)
		checkStatus();
}

void Framebuffer::forceSize(const vec2i& sz)
{
	_description.size = sz;
}

void Framebuffer::resolveMultisampledTo(Framebuffer::Pointer framebuffer)
{
	_rc->renderState().bindFramebuffer(_id);
	
	_rc->renderState().bindReadFramebuffer(_id);
	_rc->renderState().bindDrawFramebuffer(framebuffer->glID());
	
#if (ET_PLATFORM_IOS)

	glResolveMultisampleFramebufferAPPLE();
	checkOpenGLError("glResolveMultisampleFramebuffer");
	
#elif (ET_PLATFORM_ANDROID)
	
	assert(false);
	
#else
	
	glBlitFramebuffer(0, 0, _description.size.x, _description.size.y, 0, 0, framebuffer->size().x,
		framebuffer->size().y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	checkOpenGLError("glBlitFramebuffer");
	
#endif
}

void Framebuffer::setColorRenderbuffer(uint32_t r)
{
	_colorRenderbuffer = r;
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, colorAttachments[0], GL_RENDERBUFFER, _colorRenderbuffer);
	checkOpenGLError("glFramebufferRenderbuffer");
}

void Framebuffer::setDepthRenderbuffer(uint32_t r)
{
	_depthRenderbuffer = r;
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthRenderbuffer);
	checkOpenGLError("glFramebufferRenderbuffer");
}

/*
 * Support
 */
std::string FramebufferStatusToString(uint32_t status)
{
	switch (status)
	{
		case GL_FRAMEBUFFER_COMPLETE:
			return "GL_FRAMEBUFFER_COMPLETE";
			
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
			
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
			
		case GL_FRAMEBUFFER_UNSUPPORTED:
			return "GL_FRAMEBUFFER_UNSUPPORTED";
			
#if defined(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
#endif

#if defined(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_APPLE";
#endif
			
#if defined(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
#endif
			
#if defined(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
#endif
			
#if defined(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
#endif
			
		default:
			return "Unknown FBO status " + intToStr(status);
	}
}

const uint32_t colorAttachments[Framebuffer::MaxRenderTargets] =
{
	GL_COLOR_ATTACHMENT0,
	
#if defined(GL_COLOR_ATTACHMENT1)
	GL_COLOR_ATTACHMENT1,
#else
	GL_COLOR_ATTACHMENT0,
#endif
	
#if defined(GL_COLOR_ATTACHMENT2)
	GL_COLOR_ATTACHMENT2,
#else
	GL_COLOR_ATTACHMENT0,
#endif
	
#if defined(GL_COLOR_ATTACHMENT3)
	GL_COLOR_ATTACHMENT3,
#else
	GL_COLOR_ATTACHMENT0,
#endif
	
#if defined(GL_COLOR_ATTACHMENT4)
	GL_COLOR_ATTACHMENT4,
#else
	GL_COLOR_ATTACHMENT0,
#endif
	
#if defined(GL_COLOR_ATTACHMENT5)
	GL_COLOR_ATTACHMENT5,
#else
	GL_COLOR_ATTACHMENT0,
#endif
	
#if defined(GL_COLOR_ATTACHMENT6)
	GL_COLOR_ATTACHMENT6,
#else
	GL_COLOR_ATTACHMENT0,
#endif
	
#if defined(GL_COLOR_ATTACHMENT7)
	GL_COLOR_ATTACHMENT7,
#else
	GL_COLOR_ATTACHMENT0,
#endif
};