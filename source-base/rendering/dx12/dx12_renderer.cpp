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

VertexBuffer::Pointer DX12Renderer::createVertexBuffer(const std::string& name, VertexStorage::Pointer vs, BufferDrawType dt)
{
	return DX12VertexBuffer::Pointer::create(vs->declaration(), dt, name);
}

IndexBuffer::Pointer DX12Renderer::createIndexBuffer(const std::string& name, IndexArray::Pointer ia, BufferDrawType dt)
{
	return DX12IndexBuffer::Pointer::create(ia, dt, name);
}

VertexArrayObject::Pointer DX12Renderer::createVertexArrayObject(const std::string& name)
{
	return VertexArrayObject::Pointer::create(name);
}

Texture::Pointer DX12Renderer::createTexture(TextureDescription::Pointer desc)
{
	return DX12Texture::Pointer::create(desc);
}

Program::Pointer DX12Renderer::createProgram(const std::string& source, const StringList& defines, const std::string& baseFolder)
{
	return DX12Program::Pointer::create();
}

PipelineState::Pointer DX12Renderer::createPipelineState(RenderPass::Pointer, Material::Pointer, VertexArrayObject::Pointer)
{
	return DX12PipelineState::Pointer::create();
}

RenderPass::Pointer DX12Renderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	return DX12RenderPass::Pointer::create(info);
}

void DX12Renderer::submitRenderPass(RenderPass::Pointer)
{

}

void DX12Renderer::drawIndexedPrimitive(PrimitiveType, IndexArrayFormat, uint32_t first, uint32_t count)
{

}

}
