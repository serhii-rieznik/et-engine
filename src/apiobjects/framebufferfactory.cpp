/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/apiobjects/framebufferfactory.h>

using namespace et;

Framebuffer::Pointer FramebufferFactory::createFramebuffer(const vec2i& size, const std::string& id,
	int32_t colorInternalformat, uint32_t colorFormat, uint32_t colorType, int32_t depthInternalformat,
	uint32_t depthFormat, uint32_t depthType, bool useRenderbuffers, int32_t samples)
{
	FramebufferDescription desc;
	
	desc.size = size;
	
	desc.colorFormat = colorFormat;
	desc.colorInternalformat = colorInternalformat;
	desc.colorType = colorType;
	
	desc.depthFormat = depthFormat;
	desc.depthInternalformat = depthInternalformat;
	desc.depthType = depthType;
	
	desc.numColorRenderTargets = 1;
	desc.numSamples = samples;
	
	desc.colorIsRenderbuffer = (samples > 1) || useRenderbuffers;
	desc.depthIsRenderbuffer = (samples > 1) || useRenderbuffers;
	
	return Framebuffer::Pointer(new Framebuffer(renderContext(), desc, id));
}

Framebuffer::Pointer FramebufferFactory::createMultisampledFramebuffer(const vec2i& size, int32_t samples,
	const std::string& objectId, int32_t colorInternalformat, int32_t depthInternalformat)
{
	FramebufferDescription desc;
	
	desc.size = size;
	desc.colorInternalformat = colorInternalformat;
	desc.depthInternalformat = depthInternalformat;
	desc.colorFormat = 0;
	desc.colorType = 0;
	desc.depthFormat = 0;
	desc.depthType = 0;
	desc.numColorRenderTargets = 1;
	desc.numSamples = samples;
	desc.colorIsRenderbuffer = true;
	desc.depthIsRenderbuffer = true;
	
	return Framebuffer::Pointer(new Framebuffer(renderContext(), desc, objectId));
}


Framebuffer::Pointer FramebufferFactory::createCubemapFramebuffer(size_t size, const std::string& id,
	int32_t colorInternalformat, uint32_t colorFormat, uint32_t colorType, int32_t depthInternalformat,
	uint32_t depthFormat, uint32_t depthType)
{
	FramebufferDescription desc;
	
	desc.size = vec2i(static_cast<int>(size & 0xffffffff));
	
	desc.colorFormat = colorFormat;
	desc.colorInternalformat = colorInternalformat;
	desc.colorType = colorType;
	
	desc.depthFormat = depthFormat;
	desc.depthInternalformat = depthInternalformat;
	desc.depthType = depthType;
	
	desc.numColorRenderTargets = 1;
	desc.numSamples = 1;

	desc.isCubemap = true;
	
	return Framebuffer::Pointer(new Framebuffer(renderContext(), desc, id));
}

Framebuffer::Pointer FramebufferFactory::createFramebufferWrapper(uint32_t fbo, const std::string& id)
{
	return Framebuffer::Pointer(new Framebuffer(renderContext(), fbo, id));
}
