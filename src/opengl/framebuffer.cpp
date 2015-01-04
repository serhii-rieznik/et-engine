/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/opengl/opengl.h>
#include <et/opengl/openglcaps.h>
#include <et/rendering/framebuffer.h>
#include <et/rendering/rendercontext.h>

using namespace et;

extern std::string FramebufferStatusToString(uint32_t status);

Framebuffer::Framebuffer(RenderContext* rc, const FramebufferDescription& desc,
	const std::string& aName) : APIObject(aName), _rc(rc), _description(desc)
{
#if defined(ET_CONSOLE_APPLICATION)
	ET_FAIL("Attempt to create Framebuffer in console application.");
#else
	checkOpenGLError("Framebuffer::Framebuffer %s", name().c_str());

	uint32_t framebuffer = 0;
	glGenFramebuffers(1, &framebuffer);
	checkOpenGLError("Framebuffer::Framebuffer -> glGenFramebuffers");
	
	setAPIHandle(framebuffer);

	_rc->renderState().bindFramebuffer(framebuffer);

	bool hasColor = (_description.numColorRenderTargets > 0) &&
		(_description.colorInternalformat != TextureFormat::Invalid) &&
		(_description.colorIsRenderbuffer || (_description.colorFormat != TextureFormat::Invalid));
	
	bool hasDepth = (_description.depthInternalformat != TextureFormat::Invalid) &&
		(_description.depthIsRenderbuffer || (_description.depthFormat != TextureFormat::Invalid));
	
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
				Texture::Pointer target;
				if (_description.isCubemap)
				{
					target = _rc->textureFactory().genCubeTexture(_description.colorInternalformat, _description.size.x,
						_description.colorFormat, _description.colorType, name() + "_color_" + intToStr(i));
				}
				else 
				{
					size_t dataSize = _description.size.square() *
						bitsPerPixelForTextureFormat(_description.colorInternalformat, _description.colorType) / 8;
					
					target = _rc->textureFactory().genTexture(TextureTarget::Texture_2D, _description.colorInternalformat,
						_description.size, _description.colorFormat, _description.colorType,
						BinaryDataStorage(dataSize, 0), name() + "_color_" + intToStr(i));
				}
				target->setWrap(rc, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
				addRenderTarget(target);
			}
		}
		checkStatus();
	}

	if (hasDepth)
	{
		if (_description.depthIsRenderbuffer)
		{
			createOrUpdateDepthRenderbuffer();
		}
		else 
		{
			Texture::Pointer depthTarget;
			if (_description.isCubemap && (OpenGLCapabilities::instance().version() == OpenGLVersion::Version_3x))
			{
				depthTarget = _rc->textureFactory().genCubeTexture(_description.depthInternalformat, _description.size.x,
					_description.depthFormat, _description.depthType, name() + "_depth");
			}
			else 
			{
				size_t dataSize = _description.size.square() *
					bitsPerPixelForTextureFormat(_description.depthInternalformat, _description.depthType) / 8;
				
				depthTarget = _rc->textureFactory().genTexture(TextureTarget::Texture_2D, _description.depthInternalformat,
					_description.size, _description.depthFormat, _description.depthType,
					BinaryDataStorage(dataSize, 0), name() + "_depth");
			}
			depthTarget->setWrap(rc, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
			setDepthTarget(depthTarget);
		}
		checkStatus();
	}

#	if (!ET_OPENGLES)
	if (!hasColor)
	{
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);
	}
#	endif
	
#endif
}

Framebuffer::Framebuffer(RenderContext* rc, uint32_t fboId, const std::string& aName) :
	APIObject(aName), _rc(rc)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (glIsFramebuffer(fboId))
	{
		setAPIHandle(fboId);
		rc->renderState().bindFramebuffer(fboId);
		
		GLint attachmentType = 0;
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachmentType);
		checkOpenGLError("glGetFramebufferAttachmentParameteriv(GL_RENDERBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, ...)");
		
		if (attachmentType == GL_RENDERBUFFER)
		{
			glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &_description.size.x);
			checkOpenGLError("glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, ...)");
			glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_description.size.y);
			checkOpenGLError("glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, ...)");
		}
		else if (attachmentType == GL_TEXTURE)
		{
			log::info("Unable to determine Framebuffer %s dimensions.", aName.c_str());
		}
	}
	else if (fboId == 0)
	{
		_description.size = rc->sizei();
	}
#endif
}

Framebuffer::~Framebuffer()
{
#if !defined(ET_CONSOLE_APPLICATION)
	uint32_t framebuffer = static_cast<uint32_t>(apiHandle());
	_rc->renderState().frameBufferDeleted(framebuffer);
	
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
	
	if (glIsFramebuffer(framebuffer))
	{
		glDeleteFramebuffers(1, &framebuffer);
		checkOpenGLError("glDeleteFramebuffers");
	}
#endif
}

bool Framebuffer::checkStatus()
{
#if (!defined(ET_CONSOLE_APPLICATION) && ET_DEBUG)
	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));
	
	uint32_t status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	
	if (status != GL_FRAMEBUFFER_COMPLETE)
		log::error("%s for %s", FramebufferStatusToString(status).c_str(), name().c_str());
	
	return status == GL_FRAMEBUFFER_COMPLETE;
#else
	
	return true;
	
#endif
}

void Framebuffer::addRenderTarget(const Texture::Pointer& rt)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (rt.invalid() || (rt->size() != _description.size)) return;
	ET_ASSERT(glIsTexture(static_cast<uint32_t>(rt->apiHandle())));

	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));
	
	auto target = drawBufferTarget(_renderTargets.size());

	if (rt->target() == TextureTarget::Texture_2D)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, static_cast<uint32_t>(rt->apiHandle()), 0);
		checkOpenGLError("glFramebufferTexture2D(...) - %s", name().c_str());
	}
	else if (rt->target() == TextureTarget::Texture_Cube)
	{
		for (GLenum i = 0; i < 6; ++i)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, static_cast<uint32_t>(rt->apiHandle()), 0);
			checkOpenGLError("glFramebufferTexture2D(...) - %s", name().c_str());
		}
	}
	
	_renderTargets.push_back(rt);
	checkStatus();
#endif
}

void Framebuffer::setDepthTarget(const Texture::Pointer& rt)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (rt.invalid() || (rt->size() != _description.size)) return;

	_depthBuffer = rt;
	
	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));
	if (_depthBuffer->target() == TextureTarget::Texture_2D)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, static_cast<uint32_t>(_depthBuffer->apiHandle()), 0);
		checkOpenGLError("glFramebufferTexture2D");
	}
	else if (_depthBuffer->target() == TextureTarget::Texture_Cube)
	{
		for (GLenum i = 0; i < 6; ++i)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, static_cast<uint32_t>(_depthBuffer->apiHandle()), 0);
			checkOpenGLError("glFramebufferTexture2D(...) - %s", name().c_str());
		}
	}
#endif
}

void Framebuffer::setDepthTarget(const Texture::Pointer& texture, uint32_t target)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (texture.invalid() || (texture->size() != _description.size)) return;
	
	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, static_cast<uint32_t>(texture->apiHandle()), 0);
	checkOpenGLError("glFramebufferTexture2D(...) - %s", name().c_str());
#endif
}

void Framebuffer::addSameRendertarget()
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (_renderTargets.empty()) return;
		
	Texture::Pointer basic = _renderTargets.front();
	
	std::string texName = name() + "_color_" + intToStr(_renderTargets.size() + 1);
	
	Texture::Pointer target;
	if (_description.isCubemap)
	{
		target = _rc->textureFactory().genCubeTexture(basic->internalFormat(), basic->width(),
			basic->format(), basic->dataType(), texName);
	}
	else
	{
		BinaryDataStorage emptyData(basic->size().square() *
			bitsPerPixelForTextureFormat(basic->internalFormat(), basic->dataType()) / 8, 0);
		
		target = _rc->textureFactory().genTexture(basic->target(), basic->internalFormat(),
			basic->size(), basic->format(), basic->dataType(), emptyData, texName);
	}
	
	target->setWrap(_rc, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	addRenderTarget(target);
#endif 
}

void Framebuffer::setCurrentRenderTarget(const Texture::Pointer& texture)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(texture.valid());
	setCurrentRenderTarget(texture, texture->target());
#endif
}

void Framebuffer::setCurrentRenderTarget(const Texture::Pointer& texture, TextureTarget target)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(texture.valid());
	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));

	if (target == TextureTarget::Texture_Cube)
	{
		for (GLenum i = 0; i < 6; ++i)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, static_cast<uint32_t>(texture->apiHandle()), 0);
			checkOpenGLError("glFramebufferTexture2D");
		}
	}
	else if (target == TextureTarget::Texture_2D)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, static_cast<uint32_t>(texture->apiHandle()), 0);
		checkOpenGLError("glFramebufferTexture2D");
	}
	else
	{
		ET_FAIL_FMT("Invalid framebuffer attachment target: %u", static_cast<uint32_t>(target));
	}
#endif
}

void Framebuffer::setCurrentRenderTarget(size_t index)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(index < _renderTargets.size());
	ET_ASSERT(_renderTargets[index].valid());
	
	setCurrentRenderTarget(_renderTargets.at(index));
#endif
}

void Framebuffer::setCurrentCubemapFace(uint32_t faceIndex)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(_description.isCubemap && (faceIndex < 6));
	
	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));
	
	uint32_t target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex;
	
	if (_renderTargets[0].valid())
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, static_cast<uint32_t>(_renderTargets[0]->apiHandle()), 0);
		checkOpenGLError("setCurrentCubemapFace -> color");
	}
	
	if (_depthBuffer.valid())
	{
		if (OpenGLCapabilities::instance().version() == OpenGLVersion::Version_2x)
			target = GL_TEXTURE_2D;
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, static_cast<uint32_t>(_depthBuffer->apiHandle()), 0);
		checkOpenGLError("setCurrentCubemapFace -> depth");
	}
#endif
}

void Framebuffer::createOrUpdateColorRenderbuffer()
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (!glIsRenderbuffer(_colorRenderbuffer))
	{
		glGenRenderbuffers(1, &_colorRenderbuffer);
		checkOpenGLError("glGenRenderbuffers");
	}
	
	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));
	_rc->renderState().bindRenderbuffer(_colorRenderbuffer);

	if (_description.numSamples > 1)
	{
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, _description.numSamples,
			textureFormatValue(_description.colorInternalformat), _description.size.x, _description.size.y);
		checkOpenGLError("glRenderbufferStorageMultisample");
	}
	else
	{
		glRenderbufferStorage(GL_RENDERBUFFER, textureFormatValue(_description.colorInternalformat),
			_description.size.x, _description.size.y);
		checkOpenGLError("glRenderbufferStorage");
	}
	
	setColorRenderbuffer(_colorRenderbuffer);
#endif
}

void Framebuffer::createOrUpdateDepthRenderbuffer()
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (!glIsRenderbuffer(_depthRenderbuffer))
	{
		glGenRenderbuffers(1, &_depthRenderbuffer);
		checkOpenGLError("glGenRenderbuffers");
	}
	
	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));
	_rc->renderState().bindRenderbuffer(_depthRenderbuffer);
	
	if (_description.numSamples > 1)
	{
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, _description.numSamples,
			textureFormatValue(_description.depthInternalformat), _description.size.x, _description.size.y);
		checkOpenGLError("glRenderbufferStorageMultisample");
	}
	else
	{
		glRenderbufferStorage(GL_RENDERBUFFER, textureFormatValue(_description.depthInternalformat),
			_description.size.x, _description.size.y);
		checkOpenGLError("glRenderbufferStorage");
	}
	
	setDepthRenderbuffer(_depthRenderbuffer);
#endif
}

void Framebuffer::resize(const vec2i& sz)
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (_description.size == sz) return;
	
	_description.size = sz;
	
	bool hasColor = (_description.numColorRenderTargets > 0) &&
		(_description.colorInternalformat != TextureFormat::Invalid) &&
		(_description.colorIsRenderbuffer || (_description.colorFormat != TextureFormat::Invalid));
	
	bool hasDepth = (_description.depthInternalformat != TextureFormat::Invalid) &&
		(_description.depthIsRenderbuffer || (_description.depthFormat != TextureFormat::Invalid));

	if (hasColor)
	{
		if (_description.colorIsRenderbuffer)
		{
			createOrUpdateColorRenderbuffer();
		}
		else
		{
			for (auto rt : _renderTargets)
			{
				TextureDescription::Pointer desc = rt->description();
				desc->size = sz;
				desc->data.resize(desc->layersCount * desc->dataSizeForAllMipLevels());
				rt->updateData(_rc, desc);
				setCurrentRenderTarget(rt, rt->target());
				
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
			desc->data.resize(desc->layersCount * desc->dataSizeForAllMipLevels());
			_depthBuffer->updateData(_rc, desc);
			setDepthTarget(_depthBuffer);
		}
	}
	
	if (hasColor || hasDepth)
		checkStatus();
#endif
}

void Framebuffer::forceSize(const vec2i& sz)
{
	_description.size = sz;
}

void Framebuffer::resolveMultisampledTo(Framebuffer::Pointer framebuffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_rc->renderState().bindReadFramebuffer(static_cast<uint32_t>(apiHandle()));
	_rc->renderState().bindDrawFramebuffer(static_cast<uint32_t>(framebuffer->apiHandle()));
	
#	if (ET_PLATFORM_IOS)

#	if defined(GL_ES_VERSION_3_0)
		glBlitFramebuffer(0, 0, _description.size.x, _description.size.y, 0, 0, framebuffer->size().x,
			framebuffer->size().y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		checkOpenGLError("glBlitFramebuffer");
#	else
		glResolveMultisampleFramebufferAPPLE();
		checkOpenGLError("glResolveMultisampleFramebuffer");
#	endif
#	elif (ET_PLATFORM_ANDROID)
		glBlitFramebuffer(0, 0, _description.size.x, _description.size.y, 0, 0, framebuffer->size().x,
			framebuffer->size().y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		checkOpenGLError("glBlitFramebuffer");
#	else
		glBlitFramebuffer(0, 0, _description.size.x, _description.size.y, 0, 0, framebuffer->size().x,
			framebuffer->size().y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		checkOpenGLError("glBlitFramebuffer");
#	endif
	
#endif
}

void Framebuffer::invalidate(bool color, bool depth)
{
#if (ET_PLATFORM_IOS) && !defined(ET_CONSOLE_APPLICATION)
	
	_rc->renderState().bindReadFramebuffer(static_cast<uint32_t>(apiHandle()));
	
	GLsizei numDiscards = 0;
	GLenum discards[2] = { };
	
	if (color)
		discards[numDiscards++] = GL_COLOR_ATTACHMENT0;
	
	if (depth)
		discards[numDiscards++] = GL_DEPTH_ATTACHMENT;
	
#	if defined(GL_ES_VERSION_3_0)
		glInvalidateFramebuffer(GL_FRAMEBUFFER, numDiscards, discards);
		checkOpenGLError("glInvalidateFramebuffer");
#	else
		glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER, numDiscards, discards);
		checkOpenGLError("glDiscardFramebufferEXT");
#	endif
	
#else
	(void)color;
	(void)depth;
#endif
}

void Framebuffer::setColorRenderbuffer(uint32_t r)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_colorRenderbuffer = r;
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorRenderbuffer);
	checkOpenGLError("glFramebufferRenderbuffer");
#endif
}

void Framebuffer::setDepthRenderbuffer(uint32_t r)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_depthRenderbuffer = r;
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthRenderbuffer);
	checkOpenGLError("glFramebufferRenderbuffer");
#endif
}

void Framebuffer::setDrawBuffersCount(int32_t value)
{
	_drawBuffers = value;
	
	if (_rc->renderState().actualState().boundFramebuffer == apiHandle())
		_rc->renderState().setDrawBuffersCount(value);
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
