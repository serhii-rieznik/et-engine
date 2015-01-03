/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/apiobjects/framebufferfactory.h>

using namespace et;

Framebuffer::Pointer FramebufferFactory::createFramebuffer(const vec2i& size, const std::string& id,
	TextureFormat colorInternalformat, TextureFormat colorFormat, DataType colorType, TextureFormat depthInternalformat,
	TextureFormat depthFormat, DataType depthType, bool useRenderbuffers, int32_t samples)
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
	
	return Framebuffer::Pointer::create(renderContext(), desc, id);
}

Framebuffer::Pointer FramebufferFactory::createMultisampledFramebuffer(const vec2i& size, int32_t samples,
	const std::string& objectId, TextureFormat colorInternalformat, TextureFormat depthInternalformat)
{
	FramebufferDescription desc;
	
	desc.size = size;
	desc.colorInternalformat = colorInternalformat;
	desc.depthInternalformat = depthInternalformat;
	desc.numColorRenderTargets = 1;
	desc.numSamples = samples;
	desc.colorIsRenderbuffer = true;
	desc.depthIsRenderbuffer = true;
	
	return Framebuffer::Pointer::create(renderContext(), desc, objectId);
}


Framebuffer::Pointer FramebufferFactory::createCubemapFramebuffer(size_t size, const std::string& id,
	TextureFormat colorInternalformat, TextureFormat colorFormat, DataType colorType,
	TextureFormat depthInternalformat, TextureFormat depthFormat, DataType depthType)
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
	
	return Framebuffer::Pointer::create(renderContext(), desc, id);
}

Framebuffer::Pointer FramebufferFactory::createFramebufferWrapper(uint32_t fbo, const std::string& id)
{
	return Framebuffer::Pointer::create(renderContext(), fbo, id);
}
