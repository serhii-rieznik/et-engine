/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/framebuffer.h>
#include <et/rendering/program.h>
#include <et/rendering/texture.h>
#include <et/rendering/vertexarrayobject.h>

namespace et
{
	class RenderContext;
	class VertexBuffer;
	class VertexArrayObjectData;
	class IndexBufferData;
	class VertexDeclaration;
	class VertexElement;
	class IndexBuffer;
	
	typedef IntrusivePtr<VertexBuffer> VertexBufferPointer;
	typedef IntrusivePtr<IndexBuffer> IndexBufferPointer;
	typedef IntrusivePtr<VertexArrayObjectData> VertexArrayObject;
	
	class RenderState
	{
	public:
		struct Descriptor
		{
			DepthState depth;
			BlendState blend;
			RasterizerState rasterizer;
			RenderStateCache cache;
			Descriptor();
		};
		
	public:
		void setRenderContext(RenderContext* rc);
		void applyState(const Descriptor& s);

		/*
		 * Viewport
		 */
		const recti& viewport() const
			{ return _desc.cache.viewport; }
		const vec2i& viewportSize() const
			{ return _desc.cache.viewport.size(); }
		
		const recti& mainViewport() const
			{ return _mainViewport; }
		const vec2i& mainViewportSize() const
			{ return _mainViewport.size(); }

		void setMainViewportSize(const vec2i& sz, bool force = false);
		void setViewportSize(const vec2i& sz, bool force = false);

		/*
		 * Framebuffers
		 */
		Framebuffer::Pointer defaultFramebuffer()
			{ return _defaultFramebuffer; }
		
		const Framebuffer::Pointer& defaultFramebuffer() const
			{ return _defaultFramebuffer; }
		
		void setDefaultFramebuffer(const Framebuffer::Pointer& framebuffer);
		
		void bindDefaultFramebuffer(bool force = false);
		
		void bindReadFramebuffer(uint32_t framebuffer, bool force = false);
		void bindDrawFramebuffer(uint32_t framebuffer, bool force = false);
		
		void bindFramebuffer(uint32_t framebuffer, bool force = false);
		void bindFramebuffer(const Framebuffer::Pointer& fbo, bool force = false);
		
		void bindRenderbuffer(uint32_t, bool force = false);
		
		void setDrawBuffersCount(uint32_t);

		/*
		 * Textures
		 */
		void setActiveTextureUnit(uint32_t unit, bool force = false);
		void bindTexture(uint32_t unit, uint32_t texture, TextureTarget target, bool force = false);
		void bindTexture(uint32_t unit, const Texture::Pointer& texture, bool force = false);

		/*
		 * Programs
		 */
		void bindProgram(uint32_t program, bool force = false);
		void bindProgram(const Program::Pointer& prog, bool force = false);

		/*
	 	 * Buffers
		 */
		void resetBufferBindings();
		void bindBuffer(uint32_t target, uint32_t buffer, bool force = false);
		void bindVertexArray(uint32_t buffer, bool force = false);

		void bindBuffer(const VertexBufferPointer& buf, bool force = false);
		void bindBuffer(const IndexBufferPointer& buf, bool force = false);
		void bindBuffers(const VertexBufferPointer& vb, const IndexBufferPointer& ib, bool force = false);
		void bindVertexArray(const VertexArrayObject& vao, bool force = false);

		void setVertexAttributes(const VertexDeclaration& decl, bool force = false);
		void setVertexAttributesBaseIndex(const VertexDeclaration& decl, uint32_t index, bool force = false);
		void setVertexAttribEnabled(uint32_t attrib, bool enabled, bool force = false);
		void setVertexAttribPointer(const VertexElement& e, uint32_t baseIndex, bool force = false);

		/*
		 * State
		 */
		void setDepthState(const DepthState&, bool force = false);
		void setDepthFunc(CompareFunction func, bool force = false);
		void setDepthTestEnabled(bool enable, bool force = false);
		void setDepthWriteEnabled(bool enable, bool force = false);
		void setDepthClearValue(float depth, bool force = false);
		
		void setBlendState(const BlendState&, bool force = false);
		void setBlendConfiguration(BlendConfiguration blend, bool force = false);
		
		uint32_t boundFramebuffer() const
			{ return _desc.cache.boundFramebuffer; }

		uint32_t boundRenderbuffer() const
			{ return _desc.cache.boundRenderbuffer; }
		
		const BlendState& blendState() const
			{ return _desc.blend; }
		
		const DepthState& depthState() const
			{ return _desc.depth; }
		
		CompareFunction depthFunction() const
			{ return _desc.depth.compareFunction; }

		bool wireframeRendering() const 
			{ return _desc.rasterizer.fillMode == FillMode::Wireframe; }

		bool scissorEnabled() const
			{ return _desc.rasterizer.scissorEnabled; }

		const recti& clipRect() const 
			{ return _desc.rasterizer.scissorRectangle; }

		uint32_t colorMask() const
			{ return _desc.rasterizer.colorMask; }
		
		CullMode cullMode() const
			{ return _desc.rasterizer.cullMode; }
		
		vec4 clearColor() const
			{ return _desc.rasterizer.clearColor; }
		
		void setCulling(CullMode cull, bool force = false);
		void setDepthBias(bool enabled, float bias = 0.0f, float slopeScale = 0.0f, bool force = false);
		void setFillMode(FillMode mode, bool force = false);
		void setClearColor(const vec4& color, bool force = false);
		void setColorMask(uint32_t mask, bool force = false);
		void setScissor(bool enable, const recti& clip, bool force = false);
		void setSampleAlphaToCoverage(bool enable, bool force = false);
		
		/*
		 * Deletion handlers
		 */
		void programDeleted(uint32_t);
		void textureDeleted(uint32_t);
		void vertexArrayDeleted(uint32_t);
		void vertexBufferDeleted(uint32_t);
		void indexBufferDeleted(uint32_t);
		void frameBufferDeleted(uint32_t);
		
		/*
		 * Service
		 */
		const Descriptor& actualState() const
			{ return _desc; }
		
		static Descriptor currentState();
		static BlendState currentBlendState();
		static DepthState currentDepthState();
		static RasterizerState currentRasterizedState();
		static RenderStateCache currentCacheValues();
		
	protected:
		void bindFramebuffer(uint32_t framebuffer, uint32_t target, bool force);
		
	private:
		friend class RenderContext;
		
		RenderState() :
			_rc(nullptr) { }
		
		RenderState(const RenderState&) :
			_rc(nullptr) { }
		
		RenderState& operator = (const RenderState&)
			{ return *this; }
		
	private:
		RenderContext* _rc;
		Framebuffer::Pointer _defaultFramebuffer;
		Descriptor _desc;
		recti _mainViewport = recti(0, 0, 0, 0);
	};
	
	class PreservedRenderStateScope
	{
	public:
		PreservedRenderStateScope(RenderContext*, bool shouldApplyBefore);
		~PreservedRenderStateScope();

	private:
		ET_DENY_COPY(PreservedRenderStateScope)
		
	private:
		RenderContext* _rc = nullptr;
		RenderState::Descriptor _desc;
	};

}
