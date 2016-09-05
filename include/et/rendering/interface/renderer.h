/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/app/context.h>
#include <et/rendering/rendercontextparams.h>
#include <et/rendering/interface/renderpass.h>
#include <et/rendering/interface/pipelinestate.h>

namespace et
{
	class RenderContext;
	class RenderInterface : public Shared
	{
	public:
		ET_DECLARE_POINTER(RenderInterface);

	public:
		RenderInterface(RenderContext* rc)
			: _rc(rc) { }

		virtual ~RenderInterface() = default;

		RenderContext* rc() const
			{ return _rc; }

		virtual RenderingAPI api() const = 0;

		virtual void init(const RenderContextParameters& params) = 0;
		virtual void shutdown() = 0;

		virtual void begin() = 0;
		virtual void present() = 0;

		virtual RenderPass::Pointer allocateRenderPass(const RenderPass::ConstructionInfo&) = 0;
		virtual void submitRenderPass(RenderPass::Pointer) = 0;

		virtual void drawIndexedPrimitive(PrimitiveType, IndexArrayFormat, uint32_t first, uint32_t count) = 0;

		/*
		 * Vertex buffes
		 */
		virtual VertexBuffer::Pointer createVertexBuffer(const std::string&, VertexStorage::Pointer, BufferDrawType) = 0;
		virtual IndexBuffer::Pointer createIndexBuffer(const std::string&, IndexArray::Pointer, BufferDrawType) = 0;
		virtual VertexArrayObject::Pointer createVertexArrayObject(const std::string&) = 0;
        
        /*
         * Textures
         */
        virtual Texture::Pointer createTexture(TextureDescription::Pointer) = 0;

		inline Texture::Pointer loadTexture(const std::string& fileName, ObjectsCache& cache)
		{
			TextureDescription::Pointer desc = TextureDescription::Pointer::create();
			desc->load(fileName);
			return createTexture(desc);
		}
        
		/*
         * Programs
         */
        virtual Program::Pointer createProgram(const std::string& source,
            const StringList& defines, const std::string& baseFolder) = 0;
        
        inline Program::Pointer createProgram(const std::string& source)
            { return createProgram(source, emptyStringList(), emptyString); }

		/*
		 * Pipeline state
		 */
        virtual PipelineState::Pointer createPipelineState(RenderPass::Pointer, Material::Pointer, VertexArrayObject::Pointer) = 0;

	private:
		RenderContext* _rc = nullptr;
	};

/*
	class RenderContext;
	class Renderer : public RendererInterface
	{
	public:
		Renderer(RenderContext*);

	private:
		void clear(bool color = true, bool depth = true);

		void fullscreenPass();
		
		void renderFullscreenTexture(const Texture::Pointer&, const vec4& = vec4(1.0f));
		void renderFullscreenTexture(const Texture::Pointer&, const vec2& scale, const vec4& = vec4(1.0f));

		void renderFullscreenDepthTexture(const Texture::Pointer&, float factor);

		void renderTexture(const Texture::Pointer&, const vec2& position, const vec2& size,
			const vec4& = vec4(1.0f));
		void renderTextureRotated(const Texture::Pointer&, float, const vec2& position, const vec2& size,
			const vec4& = vec4(1.0f));
		
		void renderTexture(const Texture::Pointer&, const vec2i& position, const vec2i& size = vec2i(-1),
			const vec4& = vec4(1.0f));
		void renderTextureRotated(const Texture::Pointer&, float angle, const vec2i& position,
			const vec2i& size = vec2i(-1), const vec4& = vec4(1.0f));

		void drawElements(const IndexBuffer::Pointer& ib, uint32_t first, uint32_t count);
		void drawElements(PrimitiveType primitiveType, const IndexBuffer::Pointer& ib, uint32_t first, uint32_t count);
		void drawAllElements(const IndexBuffer::Pointer& ib);

		void drawElementsInstanced(const IndexBuffer::Pointer& ib, uint32_t first, uint32_t count, uint32_t instances);
		void drawElementsBaseIndex(const VertexArrayObject::Pointer& vao, int base, uint32_t first, uint32_t count);
		void drawElementsSequentially(PrimitiveType, uint32_t first, uint32_t count);
		
		BinaryDataStorage readFramebufferData(const vec2i&, TextureFormat, DataFormat);
		void readFramebufferData(const vec2i&, TextureFormat, DataFormat, BinaryDataStorage&);

		vec2 currentViewportCoordinatesToScene(const vec2i& coord);
		vec2 currentViewportSizeToScene(const vec2i& size);
		
		void finishRendering();

		uint32_t defaultTextureBindingUnit() const
			{ return _defaultTextureBindingUnit; }

		void setDefaultTextureBindingUnit(uint32_t defaultTextureBindingUnit)
			{ _defaultTextureBindingUnit = defaultTextureBindingUnit; }

	private:
		Renderer(Renderer&&) = delete;
		Renderer(const Renderer&) = delete;
		Renderer& operator = (const Renderer&) = delete;

	private:
		RenderContext* _rc = nullptr;
		uint32_t _defaultTextureBindingUnit = 7;
		
		ObjectsCache _sharedCache;
		VertexArrayObject::Pointer _fullscreenQuadVao;
		
		Program::Pointer _fullscreenProgram[TextureTarget_max];
		Program::Pointer _fullscreenDepthProgram;
		Program::Pointer _fullscreenScaledProgram;
		Program::Pointer _scaledProgram;
		Program::Pointer _scaledRotatedProgram;
		
		Program::Uniform _scaledProgram_PSUniform;
		Program::Uniform _scaledProgram_TintUniform;
		Program::Uniform _scaledRotatedProgram_PSUniform;
		Program::Uniform _scaledRotatedProgram_TintUniform;
		Program::Uniform _scaledRotatedProgram_AngleUniform;
		Program::Uniform _fullScreenScaledProgram_PSUniform;
		Program::Uniform _fullScreenScaledProgram_TintUniform;
		Program::Uniform _fullScreenDepthProgram_FactorUniform;
	};
*/
}
