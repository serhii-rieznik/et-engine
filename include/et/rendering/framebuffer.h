/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/texture.h>

namespace et
{
	class RenderContext;
	class TextureFactory;
	
	struct FramebufferDescription
	{
		vec3i size;
		uint32_t numSamples = 0;
		uint32_t numLayers_deprecated = 0;

		TextureFormat colorInternalformat = TextureFormat::Invalid;
		TextureFormat colorFormat = TextureFormat::Invalid;
		DataType colorType = DataType::UnsignedChar;
		
		TextureFormat depthInternalformat = TextureFormat::Invalid;
		TextureFormat depthFormat = TextureFormat::Invalid;
		DataType depthType = DataType::UnsignedChar;

		TextureTarget target = TextureTarget::Texture_2D;
		
		bool colorIsRenderbuffer = false;
		bool depthIsRenderbuffer = false;
	};

	class Framebuffer : public APIObject
	{
	public:
		ET_DECLARE_POINTER(Framebuffer)
		
	public:
		Framebuffer(RenderContext* rc, const FramebufferDescription& desc, const std::string& name);
		Framebuffer(RenderContext* rc, uint32_t fboId, const std::string& name);
		
		~Framebuffer();
		
		void addRenderTarget(const Texture::Pointer& texture);
		void addSameRendertarget();

		void setDepthTarget(const Texture::Pointer& texture);

		void setCurrentRenderTarget(const Texture::Pointer& texture);
		void setCurrentRenderTarget(size_t index);

		void setCurrentCubemapFace(uint32_t faceIndex);
		void setCurrentLayer(uint32_t);

		void setDrawBuffersCount(int32_t value);

		bool checkStatus();
				
		size_t numRendertargets() const
			{ return _renderTargets.size(); }
		
		int32_t drawBuffersCount() const
			{ return _drawBuffers; }

		const vec2i& size() const
			{ return _description.size.xy(); }
		
		uint32_t depthRenderbuffer() const
			{ return _depthRenderbuffer; }
		
		bool hasRenderTargets() const
			{ return !_renderTargets.empty() || !_colorRenderBuffers.empty(); }

		Texture::Pointer renderTarget(size_t index = 0) const
			{ ET_ASSERT(index < _renderTargets.size()); return _renderTargets.at(index); }
		
		uint32_t renderBufferTarget(size_t index = 0) const
			{ ET_ASSERT(index < _colorRenderBuffers.size()); return _colorRenderBuffers.at(index); }
		
		Texture::Pointer depthBuffer() const
			{ return _depthBuffer; }
		
		bool isCubemap() const
			{ return _description.target == TextureTarget::Texture_Cube; }
				
		void setColorRenderbuffer(uint32_t, uint32_t);
		void setDepthRenderbuffer(uint32_t);
		
		void resize(const vec2i&);
		void forceSize(const vec2i&);
		
		void resolveMultisampledTo(Framebuffer::Pointer, bool resolveColor, bool resolveDepth);
		void invalidate(bool color, bool depth);

	private:
		friend class FramebufferFactory;

		uint32_t buildColorRenderbuffer(uint32_t);
		void createOrUpdateDepthRenderbuffer();

		void buildColorAttachment();
		void buildDepthAttachment();

		void attachTexture(Texture::Pointer, uint32_t);

		Texture::Pointer buildTexture(const vec3i&, TextureTarget, TextureFormat, 
			TextureFormat, DataType);

	private:
		RenderContext* _rc;
		FramebufferDescription _description;
		
		std::vector<Texture::Pointer> _renderTargets;
		std::vector<uint32_t> _colorRenderBuffers;
		
		Texture::Pointer _depthBuffer;
		uint32_t _depthRenderbuffer = 0;
		
		int32_t _drawBuffers = 1;
	};

}
