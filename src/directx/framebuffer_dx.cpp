/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>

#if (ET_PLATFORM_WIN && ET_DIRECTX_RENDER)

#include <et/rendering/framebuffer.h>

using namespace et;

Framebuffer::Framebuffer(RenderContext* rc, const FramebufferDescription& desc,
	const std::string& aName) : APIObject(aName), _rc(rc), _description(desc)
{
#if !defined(ET_CONSOLE_APPLICATION)
	
#endif
}

Framebuffer::Framebuffer(RenderContext* rc, uint32_t fboId, const std::string& aName) :
	APIObject(aName), _rc(rc)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

Framebuffer::~Framebuffer()
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

bool Framebuffer::checkStatus()
{
	bool result = true;

#if !defined(ET_CONSOLE_APPLICATION)
#endif

	return result;
}

void Framebuffer::addRenderTarget(const Texture::Pointer& rt)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::setDepthTarget(const Texture::Pointer& rt)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::setDepthTarget(const Texture::Pointer& texture, uint32_t target)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::addSameRendertarget()
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif 
}

void Framebuffer::setCurrentRenderTarget(const Texture::Pointer& texture)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::setCurrentRenderTarget(const Texture::Pointer& texture, TextureTarget target)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::setCurrentRenderTarget(size_t index)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::setCurrentCubemapFace(uint32_t faceIndex)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::createOrUpdateColorRenderbuffer()
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::createOrUpdateDepthRenderbuffer()
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::resize(const vec2i& sz)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::forceSize(const vec2i& sz)
{
	_description.size = sz;
}

void Framebuffer::resolveMultisampledTo(Framebuffer::Pointer framebuffer)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::invalidate(bool color, bool depth)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::setColorRenderbuffer(uint32_t r)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::setDepthRenderbuffer(uint32_t r)
{
#if !defined(ET_CONSOLE_APPLICATION)
#endif
}

void Framebuffer::setDrawBuffersCount(int32_t value)
{
}

#endif // ET_PLATFORM_WIN && ET_DIRECTX_RENDER
