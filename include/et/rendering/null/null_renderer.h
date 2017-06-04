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
class NullRenderer : public RenderInterface
{
public:
	ET_DECLARE_POINTER(NullRenderer);

public:
	NullRenderer() : RenderInterface(nullptr) {
		initInternalStructures();
	}

	~NullRenderer() {
		shutdownInternalStructures();
	}

	RenderingAPI api() const override
		{ return RenderingAPI::Null; }

	void init(const RenderContextParameters& params) override { }

	void shutdown() override { }
	
	void destroy() override { }

	void begin() override { }
	void present() override { }

	void resize(const vec2i&) override { }

	RenderPass::Pointer allocateRenderPass(const RenderPass::ConstructionInfo&) override { return RenderPass::Pointer(); }
	void submitRenderPass(RenderPass::Pointer) override { }

	uint32_t frameIndex() const override { return 0; }
	uint32_t frameNumber() const override { return 0; }

	/*
	 * Buffer
	 */
	Buffer::Pointer createBuffer(const std::string&, const Buffer::Description& desc) override { return Buffer::Pointer(); }

	/*
	 * Textures
	 */
	Texture::Pointer createTexture(const TextureDescription::Pointer&) override { return Texture::Pointer(); }
	TextureSet::Pointer createTextureSet(const TextureSet::Description&) override { return TextureSet::Pointer(); }
	
	/*
	 * Programs
	 */
	Program::Pointer createProgram(uint32_t stages, const std::string& source) override { return Program::Pointer(); }

	/*
	 * Pipeline state
	 */
	PipelineState::Pointer acquireGraphicsPipeline(const RenderPass::Pointer&, const Material::Pointer&,
		const VertexStream::Pointer&) override { return PipelineState::Pointer(); }

	/*
	 * Sampler
	 */
	Sampler::Pointer createSampler(const Sampler::Description&) override { return Sampler::Pointer(); }

	/*
	 * Compute
	 */
	Compute::Pointer createCompute(const Material::Pointer&) override { return Compute::Pointer(); }
};
}
