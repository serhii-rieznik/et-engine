/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/renderer.h>

namespace et
{
	class RenderContext;
	class MetalRendererPrivate;
	class MetalRenderer : public RenderInterface
	{
	public:
		ET_DECLARE_POINTER(MetalRenderer);

	public:
		MetalRenderer(RenderContext*);
		~MetalRenderer();

		RenderingAPI api() const override
			{ return RenderingAPI::Metal; }

		void init(const RenderContextParameters& params) override;
		void shutdown() override;

		void begin() override;
		void present() override;

		RenderPass::Pointer allocateRenderPass(const RenderPass::ConstructionInfo&) override;
		void submitRenderPass(RenderPass::Pointer) override;

		/*
		 * Vertex buffers
		 */
		VertexBuffer::Pointer createVertexBuffer(const std::string&, VertexStorage::Pointer, BufferDrawType) override;
		IndexBuffer::Pointer createIndexBuffer(const std::string&, IndexArray::Pointer, BufferDrawType) override;
		VertexArrayObject::Pointer createVertexArrayObject(const std::string& name) override;
        
        /*
         * Textures
         */
        Texture::Pointer createTexture(TextureDescription::Pointer) override;
        
        /*
         * Programs
         */
        Program::Pointer createProgram(const std::string& program, const StringList& defines, const std::string& baseFolder) override;

		/*
		 * Pipeline state
		 */
		PipelineState::Pointer createPipelineState(RenderPass::Pointer, Material::Pointer, VertexArrayObject::Pointer) override;

		/*
		 * Low level stuff
		 */
		void drawIndexedPrimitive(PrimitiveType, IndexArrayFormat, uint32_t first, uint32_t count) override;
		
	private:
		ET_DECLARE_PIMPL(MetalRenderer, 256)
	};
}
