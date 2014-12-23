/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/texture.h>

namespace et
{
	class RenderContext;
	class TextureFactory;
	
	struct FramebufferDescription
	{
		vec2i size;

		size_t numColorRenderTargets;
		int32_t numSamples;

		int32_t colorInternalformat;
		uint32_t colorFormat;
		uint32_t colorType;

		int32_t depthInternalformat;
		uint32_t depthFormat;
		uint32_t depthType;
		
		bool colorIsRenderbuffer;
		bool depthIsRenderbuffer;
		bool isCubemap;

		FramebufferDescription() :
			numColorRenderTargets(0), numSamples(0), colorInternalformat(0), colorFormat(0),
			colorType(0), depthInternalformat(0), depthFormat(0), depthType(0), colorIsRenderbuffer(false),
			depthIsRenderbuffer(false), isCubemap(false) { }
	};

	class Framebuffer : public Object
	{
	public:
		ET_DECLARE_POINTER(Framebuffer)
		
	public:
		Framebuffer(RenderContext* rc, const FramebufferDescription& desc, const std::string& name);
		Framebuffer(RenderContext* rc, uint32_t fboId, const std::string& name);
		
		~Framebuffer();
		
		void addRenderTarget(const Texture& texture);
		void addSameRendertarget();

		void setDepthTarget(const Texture& texture);
		void setDepthTarget(const Texture& texture, uint32_t target);

		void setCurrentRenderTarget(const Texture& texture);
		void setCurrentRenderTarget(const Texture& texture, uint32_t target);
		void setCurrentRenderTarget(size_t index);
		
		void setCurrentCubemapFace(uint32_t faceIndex);
		
		void setDrawBuffersCount(int32_t value);

		bool checkStatus();
				
		size_t numRendertargets() const
			{ return _renderTargets.size(); }

		uint32_t glID() const
			{ return _id; }
		
		int32_t drawBuffersCount() const
			{ return _drawBuffers; }

		vec2i size() const
			{ return _description.size; }
	
		uint32_t colorRenderbuffer() const
			{ return _colorRenderbuffer; }
		
		uint32_t depthRenderbuffer() const
			{ return _depthRenderbuffer; }
		
		bool hasRenderTargets() const
			{ return !_renderTargets.empty(); }

		Texture renderTarget(size_t index = 0) const
			{ ET_ASSERT(index < _renderTargets.size()); return _renderTargets.at(index); }
		
		Texture depthBuffer() const
			{ return _depthBuffer; }
		
		bool isCubemap() const
			{ return _description.isCubemap; }
				
		void setColorRenderbuffer(uint32_t);
		void setDepthRenderbuffer(uint32_t);
		
		void resize(const vec2i&);
		void forceSize(const vec2i&);
		
		void resolveMultisampledTo(Framebuffer::Pointer);
		void invalidate(bool color, bool depth);

	private:
		friend class FramebufferFactory;

		void createOrUpdateColorRenderbuffer();
		void createOrUpdateDepthRenderbuffer();

	private:
		RenderContext* _rc;
		FramebufferDescription _description;
		
		std::vector<Texture> _renderTargets;
		Texture _depthBuffer;
		
		uint32_t _id = 0;
		uint32_t _colorRenderbuffer = 0;
		uint32_t _depthRenderbuffer = 0;
		int32_t _drawBuffers = 1;
	};

}
