/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/apiobject.h>
#include <et/rendering/framebuffer.h>

namespace et
{
	class RenderState;
	
	class FramebufferFactory : public APIObjectFactory
	{
	public:
		ET_DECLARE_POINTER(FramebufferFactory)
		
	public:
		FramebufferFactory(RenderContext* rc) :
			APIObjectFactory(rc) { }

		Framebuffer::Pointer createFramebuffer(const vec2i& size, const std::string& name = emptyString,
			TextureFormat colorInternalformat = TextureFormat::RGBA,
			TextureFormat colorFormat = TextureFormat::RGBA,
			DataFormat colorType = DataFormat::UnsignedChar,
			TextureFormat depthInternalformat = TextureFormat::Depth,
			TextureFormat depthFormat = TextureFormat::Depth,
			DataFormat depthType = DataFormat::UnsignedInt,
			bool useRenderbuffers = false, uint32_t samples = 0);

		Framebuffer::Pointer createFramebuffer(const vec2i& size, TextureTarget textureTarget,
			const std::string& name = emptyString,
			TextureFormat colorInternalformat = TextureFormat::RGBA,
			TextureFormat colorFormat = TextureFormat::RGBA,
			DataFormat colorType = DataFormat::UnsignedChar,
			TextureFormat depthInternalformat = TextureFormat::Depth,
			TextureFormat depthFormat = TextureFormat::Depth,
			DataFormat depthType = DataFormat::UnsignedInt, const uint32_t layers = 1);

		Framebuffer::Pointer createMultisampledFramebuffer(const vec2i& size, int32_t samples,
			const std::string& name = emptyString, TextureFormat colorInternalformat = TextureFormat::RGBA8,
			TextureFormat depthInternalformat = TextureFormat::Depth16);
		
		Framebuffer::Pointer createCubemapFramebuffer(size_t size, const std::string& objectId = emptyString,
			TextureFormat colorInternalformat = TextureFormat::RGBA,
			TextureFormat colorFormat = TextureFormat::RGBA,
			DataFormat colorType = DataFormat::UnsignedChar,
			TextureFormat depthInternalformat = TextureFormat::Depth,
			TextureFormat depthFormat = TextureFormat::Depth,
			DataFormat depthType = DataFormat::UnsignedInt);

		Framebuffer::Pointer createArrayFramebuffer(const vec2i& size, uint32_t layers, const std::string& name = emptyString,
			TextureFormat colorInternalformat = TextureFormat::RGBA,
			TextureFormat colorFormat = TextureFormat::RGBA,
			DataFormat colorType = DataFormat::UnsignedChar,
			TextureFormat depthInternalformat = TextureFormat::Depth,
			TextureFormat depthFormat = TextureFormat::Depth,
			DataFormat depthType = DataFormat::UnsignedInt);

		Framebuffer::Pointer createFramebufferWrapper(uint32_t fbo, const std::string& objectId = emptyString);

	private:
		ET_DENY_COPY(FramebufferFactory)
	};

}
