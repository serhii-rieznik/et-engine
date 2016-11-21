/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_program.h>
#include <et/rendering/dx12/dx12_texture.h>
#include <et/rendering/dx12/dx12_renderpass.h>
#include <et/rendering/dx12/dx12_pipelinestate.h>
#include <et/rendering/dx12/dx12_renderer.h>

namespace et
{

void DX12Renderer::init(const RenderContextParameters&)
{
}

void DX12Renderer::shutdown()
{

}

void DX12Renderer::resize(const vec2i &)
{
}

void DX12Renderer::begin()
{

}

void DX12Renderer::present()
{
}

Buffer::Pointer DX12Renderer::createBuffer(const Buffer::Description&)
{
	return Buffer::Pointer();
}

Texture::Pointer DX12Renderer::createTexture(TextureDescription::Pointer desc)
{
	return DX12Texture::Pointer::create(desc);
}

TextureSet::Pointer DX12Renderer::createTextureSet(const TextureSet::Description&)
{
	return TextureSet::Pointer();
}

Sampler::Pointer DX12Renderer::createSampler(const Sampler::Description &)
{
	return Sampler::Pointer();
}

PipelineState::Pointer DX12Renderer::acquirePipelineState(const RenderPass::Pointer&, const Material::Pointer&, const VertexStream::Pointer&)
{
	return PipelineState::Pointer();
}

Program::Pointer DX12Renderer::createProgram(const std::string&)
{
	return DX12Program::Pointer::create();
}

RenderPass::Pointer DX12Renderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	return DX12RenderPass::Pointer::create(info);
}

void DX12Renderer::submitRenderPass(RenderPass::Pointer)
{
}

}
