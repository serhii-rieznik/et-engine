/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vulkan/vulkan_indexbuffer.h>
#include <et/rendering/vulkan/vulkan_pipelinestate.h>
#include <et/rendering/vulkan/vulkan_program.h>
#include <et/rendering/vulkan/vulkan_renderpass.h>
#include <et/rendering/vulkan/vulkan_texture.h>
#include <et/rendering/vulkan/vulkan_vertexbuffer.h>
#include <et/rendering/vulkan/vulkan_renderer.h>

namespace et
{

void VulkanRenderer::init(const RenderContextParameters&)
{
}

void VulkanRenderer::shutdown()
{

}

void VulkanRenderer::begin()
{

}

void VulkanRenderer::present()
{

}

VertexBuffer::Pointer VulkanRenderer::createVertexBuffer(const std::string& name, VertexStorage::Pointer vs, BufferDrawType dt)
{
	return VulkanVertexBuffer::Pointer::create(vs->declaration(), dt, name);
}

IndexBuffer::Pointer VulkanRenderer::createIndexBuffer(const std::string& name, IndexArray::Pointer ia, BufferDrawType dt)
{
	return VulkanIndexBuffer::Pointer::create(ia, dt, name);
}

VertexArrayObject::Pointer VulkanRenderer::createVertexArrayObject(const std::string& name)
{
	return VertexArrayObject::Pointer::create(name);
}

Texture::Pointer VulkanRenderer::loadTexture(const std::string& fileName, ObjectsCache& cache)
{
	return VulkanTexture::Pointer::create();
}

Texture::Pointer VulkanRenderer::createTexture(TextureDescription::Pointer)
{
	return VulkanTexture::Pointer::create();
}

Program::Pointer VulkanRenderer::createProgram(const std::string& vs, const std::string& fs,
	const StringList& defines, const std::string& baseFolder)
{
	return VulkanProgram::Pointer::create();
}

PipelineState::Pointer VulkanRenderer::createPipelineState(RenderPass::Pointer, Material::Pointer, VertexArrayObject::Pointer)
{
	return VulkanPipelineState::Pointer::create();
}

RenderPass::Pointer VulkanRenderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	return VulkanRenderPass::Pointer::create(info);
}

void VulkanRenderer::submitRenderPass(RenderPass::Pointer)
{

}

void VulkanRenderer::drawIndexedPrimitive(PrimitiveType, IndexArrayFormat, uint32_t first, uint32_t count)
{

}

}
