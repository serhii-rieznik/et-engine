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

	bool hasColor = (_description.colorInternalformat != TextureFormat::Invalid) &&
		(_description.colorIsRenderbuffer || (_description.colorFormat != TextureFormat::Invalid));
	
	bool hasDepth = (_description.depthInternalformat != TextureFormat::Invalid) &&
		(_description.depthIsRenderbuffer || (_description.depthFormat != TextureFormat::Invalid));
	
	if (hasColor)
	{
		if (_description.colorIsRenderbuffer)
		{
			uint32_t buffer = buildColorRenderbuffer(0);
			_colorRenderBuffers.push_back(buffer);
			setColorRenderbuffer(buffer, 0);
		}
		else
			buildColorAttachment();
	}

	if (hasDepth)
	{
		if (_description.depthIsRenderbuffer)
			createOrUpdateDepthRenderbuffer();
		else 
			buildDepthAttachment();
	}

#if (!ET_OPENGLES)
	if (!hasColor)
	{
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);
	}
#endif

	checkStatus();

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
		_description.size.xy() = rc->sizei();
	}
#endif
}

Framebuffer::~Framebuffer()
{
#if !defined(ET_CONSOLE_APPLICATION)
	uint32_t framebuffer = static_cast<uint32_t>(apiHandle());
	_rc->renderState().frameBufferDeleted(framebuffer);
	
	for (uint32_t colorBuffer : _colorRenderBuffers)
	{
		if (glIsRenderbuffer(colorBuffer))
		{
			glDeleteRenderbuffers(1, &colorBuffer);
			checkOpenGLError("glDeleteRenderbuffers");
		}
	}
	_colorRenderBuffers.clear();
	
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

Texture::Pointer Framebuffer::buildTexture(const vec3i& aSize, TextureTarget aTarget,
	TextureFormat aInternalFormat, TextureFormat aFormat, DataType aType)
{
	switch (aTarget)
	{
		case TextureTarget::Texture_2D:
		case TextureTarget::Texture_Rectangle:
		{
			size_t dataSize = aSize.xy().square() * bitsPerPixelForTextureFormat(aInternalFormat, aType) / 8;
			return _rc->textureFactory().genTexture(aTarget, aInternalFormat, aSize.xy(),
				aFormat, aType, BinaryDataStorage(dataSize, 0), name() + "_texture");
		}

		case TextureTarget::Texture_2D_Array:
		{
			size_t dataSize = aSize.z * aSize.xy().square() * bitsPerPixelForTextureFormat(aInternalFormat, aType) / 8;
			return _rc->textureFactory().genTexture2DArray(aSize, aTarget, aInternalFormat, aFormat, aType,
				BinaryDataStorage(dataSize, 0), name() + "_texture");
		}

		case TextureTarget::Texture_Cube:
		{
			return _rc->textureFactory().genCubeTexture(aInternalFormat, etMax(aSize.x, aSize.y),
				aFormat, aType, name() + "_color");
		}

		default:
			ET_FAIL_FMT("Invalid or unsupported texture target: %u", static_cast<uint32_t>(aTarget));
	}

	return Texture::Pointer();
}

void Framebuffer::buildColorAttachment()
{
	Texture::Pointer target = buildTexture(_description.size, _description.target, 
		_description.colorInternalformat, _description.colorFormat, _description.colorType);
	target->setWrap(_rc, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
	addRenderTarget(target);
}

void Framebuffer::buildDepthAttachment()
{
	Texture::Pointer target = buildTexture(_description.size, _description.target,
		_description.depthInternalformat, _description.depthFormat, _description.depthType);

	target->setWrap(_rc, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);

	setDepthTarget(target);
}

void Framebuffer::attachTexture(Texture::Pointer rt, uint32_t target)
{
	if (rt.invalid() || (rt->size() != _description.size.xy())) return;
	ET_ASSERT(glIsTexture(static_cast<uint32_t>(rt->apiHandle())));

	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));

	if ((rt->target() == TextureTarget::Texture_2D) || (rt->target() == TextureTarget::Texture_Rectangle))
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, target, textureTargetValue(rt->target()), 
			static_cast<uint32_t>(rt->apiHandle()), 0);

		checkOpenGLError("glFramebufferTexture2D(...) - %s", name().c_str());
	}
	else if (rt->target() == TextureTarget::Texture_Cube)
	{
		for (GLenum i = 0; i < 6; ++i)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
				static_cast<uint32_t>(rt->apiHandle()), 0);

			checkOpenGLError("glFramebufferTexture2D(...) - %s", name().c_str());
		}
	}
#if (!ET_OPENGLES)
	else if (rt->target() == TextureTarget::Texture_2D_Array)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, target, static_cast<GLuint>(rt->apiHandle()), 0);
		checkOpenGLError("glFramebufferTexture(...) - %s", name().c_str());
	}
#endif
}

void Framebuffer::addRenderTarget(const Texture::Pointer& rt)
{
#if !defined(ET_CONSOLE_APPLICATION)
	attachTexture(rt, drawBufferTarget(_renderTargets.size()));
	_renderTargets.push_back(rt);
	checkStatus();
	
	setDrawBuffersCount(_renderTargets.size() & 0xff);
#endif
}

void Framebuffer::setDepthTarget(const Texture::Pointer& texture)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_depthBuffer = texture;
	attachTexture(_depthBuffer, GL_DEPTH_ATTACHMENT);
#endif
}

void Framebuffer::addSameRendertarget()
{
#if !defined(ET_CONSOLE_APPLICATION)
	if (_description.colorIsRenderbuffer)
	{
		ET_ASSERT(!_colorRenderBuffers.empty());
		uint32_t newRenderTarget = buildColorRenderbuffer(0);
		_colorRenderBuffers.push_back(newRenderTarget);
		setColorRenderbuffer(newRenderTarget, 1);
	}
	else
	{
		ET_ASSERT(!_renderTargets.empty());
		
		const Texture::Pointer& basic = _renderTargets.front();
		
		Texture::Pointer target = buildTexture(vec3i(basic->size(), basic->description()->layersCount),
			basic->target(), basic->internalFormat(), basic->format(), basic->dataType());
		target->setName(name() + "_color_" + intToStr(_renderTargets.size() + 1));
		target->setWrap(_rc, TextureWrap::ClampToEdge, TextureWrap::ClampToEdge);
		addRenderTarget(target);
	}
#endif
	/*
	if (_renderTargets.empty())
	{
		if (_colorRenderbuffer == 0)
		{
			log::warning("Unable to add same render target, FBO has no valid attachments.");
			return;
		}
		
		GLuint newRenderBuffer = 0;
		glGenRenderbuffers(1, &newRenderBuffer);
		_rc->renderState().bindRenderbuffer(newRenderBuffer);
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
		setColorRenderbuffer(newRenderBuffer, 1);
	}
	else
	{
	}
#endif 
*/
}

void Framebuffer::setCurrentRenderTarget(const Texture::Pointer& texture)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(texture.valid());
	attachTexture(texture, GL_COLOR_ATTACHMENT0);
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
	ET_ASSERT((_description.target == TextureTarget::Texture_Cube) && (faceIndex < 6));
	
	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));
	
	uint32_t target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex;
	
	if (_renderTargets[0].valid())
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target,
			static_cast<uint32_t>(_renderTargets[0]->apiHandle()), 0);
		checkOpenGLError("setCurrentCubemapFace -> color");
	}
	
	if (_depthBuffer.valid())
	{
		if (OpenGLCapabilities::instance().version() == OpenGLVersion::Version_2x)
			target = GL_TEXTURE_2D;
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target,
			static_cast<uint32_t>(_depthBuffer->apiHandle()), 0);
		checkOpenGLError("setCurrentCubemapFace -> depth");
	}
#endif
}

void Framebuffer::setCurrentLayer(uint32_t layerIndex)
{
#if (!ET_OPENGLES)
	ET_ASSERT(layerIndex < static_cast<uint32_t>(_description.size.z));
	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));

	uint32_t targetIndex = 0;
	for (const auto& t : _renderTargets)
	{
		glFramebufferTextureLayer(GL_FRAMEBUFFER, drawBufferTarget(targetIndex),
			static_cast<GLuint>(t->apiHandle()), 0, layerIndex);
		++targetIndex;
	}

	if (_depthBuffer.valid())
	{
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
			static_cast<GLuint>(_depthBuffer->apiHandle()), 0, layerIndex);
	}
#endif
}

uint32_t Framebuffer::buildColorRenderbuffer(uint32_t input)
{
	uint32_t result = input;
	
#if !defined(ET_CONSOLE_APPLICATION)
	if ((result == 0) || !glIsRenderbuffer(result))
	{
		glGenRenderbuffers(1, &result);
		checkOpenGLError("glGenRenderbuffers");
	}
	
	_rc->renderState().bindFramebuffer(static_cast<uint32_t>(apiHandle()));
	_rc->renderState().bindRenderbuffer(result);

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
#endif
	
	return result;
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
	if (_description.size.xy() == sz) return;
	
	forceSize(sz);
	
	bool hasColor = (_description.colorInternalformat != TextureFormat::Invalid) &&
		(_description.colorIsRenderbuffer || (_description.colorFormat != TextureFormat::Invalid));
	
	bool hasDepth = (_description.depthInternalformat != TextureFormat::Invalid) &&
		(_description.depthIsRenderbuffer || (_description.depthFormat != TextureFormat::Invalid));

	if (hasColor)
	{
		if (_description.colorIsRenderbuffer)
		{
			for (uint32_t& renderBuffer : _colorRenderBuffers)
				renderBuffer = buildColorRenderbuffer(renderBuffer);
		}
		else
		{
			for (auto rt : _renderTargets)
			{
				TextureDescription::Pointer desc = rt->description();
				desc->size = sz;
				desc->data.resize(desc->layersCount * desc->dataSizeForAllMipLevels());
				desc->data.fill(0);
				rt->updateData(_rc, desc);
				setCurrentRenderTarget(rt);
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
			desc->data.fill(0);
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
	_description.size = vec3i(sz, _description.size.z);
}

void Framebuffer::resolveMultisampledTo(Framebuffer::Pointer framebuffer, bool resolveColor, bool resolveDepth)
{
#if !defined(ET_CONSOLE_APPLICATION)
	_rc->renderState().bindReadFramebuffer(static_cast<uint32_t>(apiHandle()));
	_rc->renderState().bindDrawFramebuffer(static_cast<uint32_t>(framebuffer->apiHandle()));
	
#	if (ET_OPENGLES)
	if (OpenGLCapabilities::instance().versionShortString() < "300")
	{
		glResolveMultisampleFramebufferAPPLE();
		checkOpenGLError("glResolveMultisampleFramebuffer");
	}
#	endif
	
	if (resolveColor)
	{
		vec2i sourceSize = _description.size.xy();
		vec2i destSize = framebuffer->size();
		
		glBlitFramebuffer(0, 0, sourceSize.x, sourceSize.y, 0, 0, destSize.x, destSize.y,
			GL_COLOR_BUFFER_BIT, (sourceSize == destSize) ? GL_NEAREST : GL_LINEAR);
		checkOpenGLError("glBlitFramebuffer");
	}

	if (resolveDepth)
	{
		glBlitFramebuffer(0, 0, _description.size.x, _description.size.y, 0, 0, framebuffer->size().x,
			framebuffer->size().y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		checkOpenGLError("glBlitFramebuffer");
	}
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

void Framebuffer::setColorRenderbuffer(uint32_t r, uint32_t index)
{
#if !defined(ET_CONSOLE_APPLICATION)
	ET_ASSERT(index < _colorRenderBuffers.size());
	_colorRenderBuffers[index] = r;
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, drawBufferTarget(index), GL_RENDERBUFFER, r);
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
