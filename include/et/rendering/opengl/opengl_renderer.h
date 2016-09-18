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
	class OpenGLRendererPrivate;
	class OpenGLRenderer : public RenderInterface
	{
	public:
		ET_DECLARE_POINTER(OpenGLRenderer);

	public:
		OpenGLRenderer(RenderContext*);
		~OpenGLRenderer();

		RenderingAPI api() const override
			{ return RenderingAPI::OpenGL; }

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

        /*
         * Textures
         */
        Texture::Pointer createTexture(TextureDescription::Pointer) override;
        
        /*
         * Programs
         */
        Program::Pointer createProgram(const std::string& vs, const std::string& fs, 
			const StringList& defines, const std::string& baseFolder) override;

		/*
		 * Pipeline state
		 */
		PipelineState::Pointer createPipelineState(RenderPass::Pointer, Material::Pointer, VertexStream::Pointer) override;

	private:
		ET_DECLARE_PIMPL(OpenGLRenderer, 256)
	};
}
