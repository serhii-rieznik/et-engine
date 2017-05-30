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
	void destroy() override;

	void begin() override;
	void present() override;

	void resize(const vec2i&) override;

	RenderPass::Pointer allocateRenderPass(const RenderPass::ConstructionInfo&) override;
	void submitRenderPass(RenderPass::Pointer) override;

	uint32_t frameIndex() const override;
	uint32_t frameNumber() const override;

	/*
	 * Buffer
	 */
	Buffer::Pointer createBuffer(const std::string&, const Buffer::Description& desc) override;
	/*
	 * Textures
	 */
	Texture::Pointer createTexture(const TextureDescription::Pointer&) override;
	TextureSet::Pointer createTextureSet(const TextureSet::Description&) override;
	
	/*
	 * Programs
	 */
	Program::Pointer createProgram(uint32_t stages, const std::string& source) override;

	/*
	 * Pipeline state
	 */
	PipelineState::Pointer acquireGraphicsPipeline(const RenderPass::Pointer&, const Material::Pointer&,
		const VertexStream::Pointer&) override;

	/*
	 * Sampler
	 */
	Sampler::Pointer createSampler(const Sampler::Description&) override;

	/*
	 * Compute
	 */
	Compute::Pointer createCompute(const Material::Pointer&) override;

private:
	ET_DECLARE_PIMPL(MetalRenderer, 512);
};
}
