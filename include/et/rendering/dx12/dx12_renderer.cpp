/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_indexbuffer.h>
#include <et/rendering/dx12/dx12_pipelinestate.h>
#include <et/rendering/dx12/dx12_program.h>
#include <et/rendering/dx12/dx12_renderpass.h>
#include <et/rendering/dx12/dx12_texture.h>
#include <et/rendering/dx12/dx12_vertexbuffer.h>
#include <et/rendering/dx12/dx12_renderer.h>

namespace et
{

void DX12Renderer::init(const RenderContextParameters&)
{
}

void DX12Renderer::shutdown()
{

}

void DX12Renderer::begin()
{

}

void DX12Renderer::present()
{

}

DataBuffer::Pointer DX12Renderer::createDataBuffer(const std::string&, uint32_t size)
{
	return DataBuffer::Pointer();
}

DataBuffer::Pointer DX12Renderer::createDataBuffer(const std::string&, const BinaryDataStorage&)
{
	return DataBuffer::Pointer();
}

VertexBuffer::Pointer DX12Renderer::createVertexBuffer(const std::string& name, VertexStorage::Pointer vs, BufferDrawType dt)
{
	return DX12VertexBuffer::Pointer::create(vs->declaration(), dt, name);
}

IndexBuffer::Pointer DX12Renderer::createIndexBuffer(const std::string& name, IndexArray::Pointer ia, BufferDrawType dt)
{
	return DX12IndexBuffer::Pointer::create(ia, dt, name);
}

Texture::Pointer DX12Renderer::createTexture(TextureDescription::Pointer desc)
{
	return DX12Texture::Pointer::create(desc);
}

Sampler::Pointer DX12Renderer::createSampler(const Sampler::Description &)
{
	return Sampler::Pointer();
}

PipelineState::Pointer DX12Renderer::acquirePipelineState(RenderPass::Pointer, Material::Pointer, VertexStream::Pointer)
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
