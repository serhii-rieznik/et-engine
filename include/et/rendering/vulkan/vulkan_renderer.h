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
class VulkanRendererPrivate;
class VulkanRenderer : public RenderInterface
{
public:
	ET_DECLARE_POINTER(VulkanRenderer);

public:
	RenderingAPI api() const override
	{
		return RenderingAPI::Vulkan;
	}

	VulkanRenderer(RenderContext* rc);
	~VulkanRenderer();

	void init(const RenderContextParameters& params) override;
	void shutdown() override;
	void destroy() override;

	void resize(const vec2i&) override;

	void begin() override;
	void present() override;

	RenderPass::Pointer allocateRenderPass(const RenderPass::ConstructionInfo&) override;
	void submitRenderPass(RenderPass::Pointer) override;

	uint32_t frameIndex() const override;
	uint32_t frameNumber() const override;

	/*
	 * Buffers
	 */
	Buffer::Pointer createBuffer(const std::string&, const Buffer::Description&) override;

	/*
	 * Textures
	 */
	Texture::Pointer createTexture(const TextureDescription::Pointer&) override;
	TextureSet::Pointer createTextureSet(const TextureSet::Description&) override;

	/*
	 * Samplers
	 */
	Sampler::Pointer createSampler(const Sampler::Description&) override;

	/*
	 * Programs
	 */
	Program::Pointer createProgram(uint32_t stages, const std::string& source) override;

	/*
	 * Pipeline state
	 */
	PipelineState::Pointer acquirePipelineState(const RenderPass::Pointer&, const Material::Pointer&, const VertexStream::Pointer&) override;

private:
	ET_DECLARE_PIMPL(VulkanRenderer, 2048);
};
}
