/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.h>
#include <et/rendering/texture.h>
#include <et/rendering/program.h>
#include <et/rendering/framebuffer.h>
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
		struct State
		{
		public:
			typedef std::map<uint32_t, uint32_t> UnitToTextureMap;
			typedef std::map<TextureTarget, UnitToTextureMap> TargetToUnitTextureMap;
			
		public:
			TargetToUnitTextureMap boundTextures;
			
			StaticDataStorage<size_t, static_cast<size_t>(VertexAttributeUsage::max)> enabledVertexAttributes;
			StaticDataStorage<size_t, MaxDrawBuffers> drawBuffers;
			
			uint32_t activeTextureUnit = 0;
			
			uint32_t boundFramebuffer = 0;
			uint32_t boundReadFramebuffer = 0;
			uint32_t boundDrawFramebuffer = 0;
			uint32_t boundRenderbuffer = 0;
			
			uint32_t boundArrayBuffer = 0;
			uint32_t boundElementArrayBuffer = 0;
			uint32_t boundVertexArrayObject = 0;
			
			uint32_t boundProgram = 0;
			
			recti clipRect;
			vec2i mainViewportSize;
			vec2i viewportSize;
			vec2 mainViewportSizeFloat;
			vec2 viewportSizeFloat;

			vec4 clearColor;
			
			size_t colorMask = 0;
			float clearDepth = false;
			
			float polygonOffsetFactor = 0.0f;
			float polygonOffsetUnits = 0.0f;
			
			bool blendEnabled = false;
			bool depthTestEnabled = false;
			bool depthMask = false;
			bool polygonOffsetFillEnabled = false;
			bool wireframe = false;
			bool clipEnabled = false;
			bool cullEnabled = false;
			bool alphaToCoverage = false;
			bool pointSizeControlInVertexShaderEnabled = false;
			
			BlendState lastColorBlend = BlendState::Current;
			BlendState lastAlphaBlend = BlendState::Current;
			CullState lastCull = CullState::Current;
			DepthFunc lastDepthFunc = DepthFunc::Less;
			
			State();
		};

	public: 
		void setRenderContext(RenderContext* rc);
		void reset();
		void applyState(const State& s);

		/*
		 * Viewport
		 */
		const vec2i& mainViewportSize() const
			{ return _currentState.mainViewportSize; }
		const vec2i& viewportSize() const
			{ return _currentState.viewportSize; }
		const vec2& mainViewportSizeFloat() const
			{ return _currentState.mainViewportSizeFloat; }
		const vec2& viewportSizeFloat() const
			{ return _currentState.viewportSizeFloat; }

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
		
		void setDrawBuffersCount(int32_t);

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
		void setVertexAttributesBaseIndex(const VertexDeclaration& decl, size_t index, bool force = false);
		void setVertexAttribEnabled(uint32_t attrib, bool enabled, bool force = false);
		void setVertexAttribPointer(const VertexElement& e, size_t baseIndex, bool force = false);

		/*
		 * State
		 */
		uint32_t boundFramebuffer() const
			{ return _currentState.boundFramebuffer; }

		uint32_t boundRenderbuffer() const
			{ return _currentState.boundRenderbuffer; }
		
		bool blendEnabled() const
			{ return _currentState.blendEnabled; }

		BlendState blendStateForColor() const
			{ return _currentState.lastColorBlend; }

		BlendState blendStateForAlpha() const
		{ return _currentState.lastAlphaBlend; }
		
		bool depthTestEnabled() const 
			{ return _currentState.depthTestEnabled; }

		bool depthMask() const 
			{ return _currentState.depthMask; }
		
		DepthFunc depthFunc() const
			{ return _currentState.lastDepthFunc; }

		bool wireframeRendering() const 
			{ return _currentState.wireframe; }

		bool clipEnabled() const 
			{ return _currentState.clipEnabled; }

		const recti& clipRect() const 
			{ return _currentState.clipRect; }

		size_t colorMask() const
			{ return _currentState.colorMask; }
		
		bool cullEnabled() const
			{ return _currentState.cullEnabled; }

		CullState cullState() const
			{ return _currentState.lastCull; }
		
		vec4 clearColor() const
			{ return _currentState.clearColor; }
		
		void setBlend(bool enable, BlendState blend = BlendState::Current, bool force = false);
		
		void setSeparateBlend(bool enable, BlendState color = BlendState::Current,
			BlendState alpha = BlendState::Current, bool force = false);
		
		void setCulling(bool enabled, CullState cull = CullState::Current, bool force = false);
		void setDepthTest(bool enable, bool force = false);
		void setDepthFunc(DepthFunc func, bool force = false);
		void setDepthMask(bool enable, bool force = false);
		void setPolygonOffsetFill(bool enabled, float factor = 0.0f, float units = 0.0f, bool force = false);
		void setWireframeRendering(bool wire, bool force = false);
		void setClearColor(const vec4& color, bool force = false);
		void setColorMask(size_t mask, bool force = false);
		void setClearDepth(float depth, bool force = false);
		void setClip(bool enable, const recti& clip, bool force = false);
		void setSampleAlphaToCoverage(bool enable, bool force = false);
		void setPointSizeControlInVertexShader(bool enable, bool force = false);

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
		const State& actualState() const
			{ return _currentState; }
		
		static State currentState();
		
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
	
		State _currentState;
	};
	
	class PreservedRenderStateScope
	{
	public:
		PreservedRenderStateScope(RenderContext*, bool shouldApplyBefore);
		~PreservedRenderStateScope();

	private:
		ET_DENY_COPY(PreservedRenderStateScope)
		
	private:
		RenderContext* _rc;
		RenderState::State _state;
	};

}
