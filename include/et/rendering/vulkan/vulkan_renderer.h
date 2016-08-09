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
	class VulkanRenderer : public RenderInterface
	{
	public:
		ET_DECLARE_POINTER(VulkanRenderer);

	public:
		RenderingAPI api() const override
			{  return RenderingAPI::Vulkan; }

		VulkanRenderer(RenderContext* rc) : 
			RenderInterface(rc) { }

		void init(const RenderContextParameters& params);
		void shutdown();

		void begin();
		void present();

		RenderPass::Pointer allocateRenderPass(const RenderPass::ConstructionInfo&);
		void submitRenderPass(RenderPass::Pointer);

		void drawIndexedPrimitive(PrimitiveType, IndexArrayFormat, uint32_t first, uint32_t count);

		/*
		 * Vertex buffes
		 */
		VertexBuffer::Pointer createVertexBuffer(const std::string&, VertexStorage::Pointer, BufferDrawType);
		IndexBuffer::Pointer createIndexBuffer(const std::string&, IndexArray::Pointer, BufferDrawType);
		VertexArrayObject::Pointer createVertexArrayObject(const std::string&);
        
        /*
         * Textures
         */
        Texture::Pointer loadTexture(const std::string& fileName, ObjectsCache& cache);
        Texture::Pointer createTexture(TextureDescription::Pointer);
        
        /*
         * Programs
         */
        Program::Pointer createProgram(const std::string& vs, const std::string& fs,
            const StringList& defines, const std::string& baseFolder);
        
        inline Program::Pointer createProgram(const std::string& vs, const std::string& fs)
            { return createProgram(vs, fs, emptyStringList(), emptyString); }

		/*
		 * Pipeline state
		 */
        PipelineState::Pointer createPipelineState(RenderPass::Pointer, Material::Pointer, VertexArrayObject::Pointer);
	};
}
