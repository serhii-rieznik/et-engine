/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/rendering/metal/metal.h>
#include <et/rendering/metal/metal_renderer.h>
#include <et/rendering/metal/metal_renderpass.h>
#include <et/rendering/metal/metal_texture.h>
#include <et/rendering/metal/metal_vertexbuffer.h>
#include <et/rendering/metal/metal_indexbuffer.h>
#include <et/rendering/metal/metal_program.h>
#include <et/rendering/metal/metal_pipelinestate.h>

namespace et
{

class MetalRendererPrivate
{
public:
	MetalState metal;
};

MetalRenderer::MetalRenderer(RenderContext* rc)
	: RenderInterface(rc)
{
	ET_PIMPL_INIT(MetalRenderer);
}

MetalRenderer::~MetalRenderer()
{
	ET_PIMPL_FINALIZE(MetalRenderer)
}

void MetalRenderer::init(const RenderContextParameters& params)
{
	_private->metal.device = MTLCreateSystemDefaultDevice();
	_private->metal.queue = [_private->metal.device newCommandQueue];

	_private->metal.layer = [[CAMetalLayer alloc] init];
	_private->metal.layer.device = _private->metal.device;
	_private->metal.layer.framebufferOnly = YES;
	_private->metal.layer.pixelFormat = MTLPixelFormatBGRA8Unorm;

	application().context().objects[3] = (__bridge void*)(_private->metal.device);
	application().context().objects[4] = (__bridge void*)_private->metal.layer;
}

void MetalRenderer::shutdown()
{
	ET_OBJC_RELEASE(_private->metal.queue);
	ET_OBJC_RELEASE(_private->metal.device);
}

void MetalRenderer::begin()
{
	_private->metal.mainDrawable = [_private->metal.layer nextDrawable];
	ET_ASSERT(_private->metal.mainDrawable != nil);

	_private->metal.mainCommandBuffer = [_private->metal.queue commandBuffer];
	ET_ASSERT(_private->metal.mainCommandBuffer != nil);
}

void MetalRenderer::present()
{
	[_private->metal.mainCommandBuffer presentDrawable:_private->metal.mainDrawable];
	[_private->metal.mainCommandBuffer commit];
	[_private->metal.mainCommandBuffer waitUntilCompleted];
	
	_private->metal.mainCommandBuffer = nil;
	_private->metal.mainDrawable = nil;
}

RenderPass::Pointer MetalRenderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	MetalRenderPass::Pointer result = MetalRenderPass::Pointer::create(this, _private->metal, info);

	return result;
}

void MetalRenderer::submitRenderPass(RenderPass::Pointer in_pass)
{
	MetalRenderPass::Pointer pass = in_pass;
	pass->endEncoding();
}

void MetalRenderer::drawIndexedPrimitive(PrimitiveType pt, IndexArrayFormat fmt, uint32_t first, uint32_t count)
{

}

/*
 * Vertex buffers
 */
VertexBuffer::Pointer MetalRenderer::createVertexBuffer(const std::string& name, VertexStorage::Pointer vs, BufferDrawType dt)
{
	return MetalVertexBuffer::Pointer::create(_private->metal, vs->declaration(), vs->data(), dt, name);
}

IndexBuffer::Pointer MetalRenderer::createIndexBuffer(const std::string& name, IndexArray::Pointer ia, BufferDrawType dt)
{
    return MetalIndexBuffer::Pointer::create(_private->metal, ia, dt, name);
}

VertexArrayObject::Pointer MetalRenderer::createVertexArrayObject(const std::string& name)
{
	return VertexArrayObject::Pointer::create(name);
}

/*
 * Textures
 */
Texture::Pointer MetalRenderer::createTexture(TextureDescription::Pointer desc)
{
    return MetalTexture::Pointer::create(_private->metal, desc);
}

/*
 * Programs
 */
Program::Pointer MetalRenderer::createProgram(const std::string& vs, const std::string& fs,
    const StringList& defines, const std::string& baseFolder)
{
    MetalProgram::Pointer program = MetalProgram::Pointer::create(_private->metal);
	program->build(vs, fs);
    return program;
}

/*
 * Pipeline state
 */
PipelineState::Pointer MetalRenderer::createPipelineState(RenderPass::Pointer pass, Material::Pointer mtl,
    VertexArrayObject::Pointer vs)
{
    PipelineState::Pointer result = MetalPipelineState::Pointer::create(_private->metal);
	result->setInputLayout(vs->vertexBuffer()->declaration());
	result->setDepthState(mtl->depthState());
    result->setBlendState(mtl->blendState());
    result->setCullMode(mtl->cullMode());
    result->setProgram(mtl->program());
    result->setVertexStream(vs);
	return result;
}

}
