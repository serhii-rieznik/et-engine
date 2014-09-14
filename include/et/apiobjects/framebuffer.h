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
		
		enum
		{
			MaxRenderTargets = 8
		};
		
	public:
		~Framebuffer();

		bool addRenderTarget(const Texture& texture);
		void addSameRendertarget();

		void setDepthTarget(const Texture& texture);
		void setDepthTarget(const Texture& texture, uint32_t target);

		void setCurrentRenderTarget(const Texture& texture);
		void setCurrentRenderTarget(const Texture& texture, uint32_t target);
		void setCurrentRenderTarget(size_t index);
		
		void setCurrentCubemapFace(uint32_t faceIndex);

		bool checkStatus();
		
		void setDrawBuffersCount(int c);
		
		size_t numRendertargets() const
			{ return _numTargets; }

		uint32_t glID() const
			{ return _id; }

		vec2i size() const
			{ return _description.size; }
	
		uint32_t colorRenderbuffer() const
			{ return _colorRenderbuffer; }
		
		uint32_t depthRenderbuffer() const
			{ return _depthRenderbuffer; }

		Texture renderTarget(size_t index = 0) const
			{ ET_ASSERT(index < MaxRenderTargets); return _renderTargets[index]; }
		
		Texture depthBuffer() const
			{ return _depthBuffer; }
				
		void setColorRenderbuffer(uint32_t);
		void setDepthRenderbuffer(uint32_t);
		
		void resize(const vec2i&);
		void forceSize(const vec2i&);
		
		void resolveMultisampledTo(Framebuffer::Pointer);
		void invalidate(bool color, bool depth);

	private:
		friend class FramebufferFactory;
		
		Framebuffer(RenderContext* rc, const FramebufferDescription& desc, const std::string& name);
		Framebuffer(RenderContext* rc, uint32_t fboId, const std::string& name);

		void createOrUpdateColorRenderbuffer();
		void createOrUpdateDepthRenderbuffer();

	private:
		RenderContext* _rc;
		FramebufferDescription _description;
		
		Texture _renderTargets[MaxRenderTargets];
		Texture _depthBuffer;

		size_t _numTargets;
		
		uint32_t _id;
		uint32_t _colorRenderbuffer;
		uint32_t _depthRenderbuffer;
	};

}
