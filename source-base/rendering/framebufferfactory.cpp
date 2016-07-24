/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/framebufferfactory.h>

using namespace et;

Framebuffer::Pointer FramebufferFactory::createFramebuffer(const vec2i& size, const std::string& id,
	TextureFormat colorInternalformat, TextureFormat colorFormat, DataFormat colorType, TextureFormat depthInternalformat,
	TextureFormat depthFormat, DataFormat depthType, bool useRenderbuffers, uint32_t samples)
{
	FramebufferDescription desc;
	desc.target = TextureTarget::Texture_2D;
	desc.size = vec3i(size, 0);
	desc.numSamples = samples;

	desc.colorFormat = colorFormat;
	desc.colorInternalformat = colorInternalformat;
	desc.colorType = colorType;
	
	desc.depthFormat = depthFormat;
	desc.depthInternalformat = depthInternalformat;
	desc.depthType = depthType;
	
	desc.colorIsRenderbuffer = (samples > 1) || useRenderbuffers;
	desc.depthIsRenderbuffer = (samples > 1) || useRenderbuffers;
	
	return Framebuffer::Pointer::create(renderContext(), desc, id);
}

Framebuffer::Pointer FramebufferFactory::createFramebuffer(const vec2i& size, TextureTarget textureTarget,
	const std::string& name, TextureFormat colorInternalformat, TextureFormat colorFormat, DataFormat colorType,
	TextureFormat depthInternalformat, TextureFormat depthFormat, DataFormat depthType, const uint32_t layers)
{
	FramebufferDescription desc;
	
	desc.target = textureTarget;
	desc.size = vec3i(size, layers);
	
	desc.colorFormat = colorFormat;
	desc.colorInternalformat = colorInternalformat;
	desc.colorType = colorType;
	
	desc.depthFormat = depthFormat;
	desc.depthInternalformat = depthInternalformat;
	desc.depthType = depthType;
	
	return Framebuffer::Pointer::create(renderContext(), desc, name);
}

Framebuffer::Pointer FramebufferFactory::createMultisampledFramebuffer(const vec2i& size, int32_t samples,
	const std::string& objectId, TextureFormat colorInternalformat, TextureFormat depthInternalformat)
{
	FramebufferDescription desc;
	
	desc.target = TextureTarget::Texture_2D;
	desc.size = vec3i(size, 0);
	desc.numSamples = samples;
	desc.colorInternalformat = colorInternalformat;
	desc.depthInternalformat = depthInternalformat;
	desc.colorIsRenderbuffer = true;
	desc.depthIsRenderbuffer = true;
	
	return Framebuffer::Pointer::create(renderContext(), desc, objectId);
}

Framebuffer::Pointer FramebufferFactory::createCubemapFramebuffer(size_t size, const std::string& id,
	TextureFormat colorInternalformat, TextureFormat colorFormat, DataFormat colorType,
	TextureFormat depthInternalformat, TextureFormat depthFormat, DataFormat depthType)
{
	FramebufferDescription desc;
	
	desc.target = TextureTarget::Texture_Cube;
	desc.size = vec3i(static_cast<int32_t>(size & 0xffffffff), static_cast<int32_t>(size & 0xffffffff), 0);
	desc.numSamples = 1;

	desc.colorFormat = colorFormat;
	desc.colorInternalformat = colorInternalformat;
	desc.colorType = colorType;
	
	desc.depthFormat = depthFormat;
	desc.depthInternalformat = depthInternalformat;
	desc.depthType = depthType;

	return Framebuffer::Pointer::create(renderContext(), desc, id);
}

Framebuffer::Pointer FramebufferFactory::createFramebufferWrapper(uint32_t fbo, const std::string& id)
{
	return Framebuffer::Pointer::create(renderContext(), fbo, id);
}

Framebuffer::Pointer FramebufferFactory::createArrayFramebuffer(const vec2i& size, const uint32_t layers, 
	const std::string& name, TextureFormat colorInternalformat, TextureFormat colorFormat, DataFormat colorType, 
	TextureFormat depthInternalformat, TextureFormat depthFormat, DataFormat depthType)
{
	return createFramebuffer(size, TextureTarget::Texture_2D_Array, name, colorInternalformat, colorFormat, colorType,
		depthInternalformat, depthFormat, depthType, layers);
}
