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
#include <et/rendering/metal/metal_databuffer.h>
#include <et/rendering/metal/metal_program.h>
#include <et/rendering/metal/metal_pipelinestate.h>

@interface CAMetalLayer(ShutUpDisplay) @end
@implementation CAMetalLayer(ShutUpDisplay) - (void)display { } @end

namespace et
{

class MetalRendererPrivate
{
public:
	MetalState metal;
	PipelineStateCache cache;
	Map<uint64_t, Sampler::Pointer> samplersCache;
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

	sharedVariables().init(this);
	sharedConstBuffer().init(this);
	sharedMaterialLibrary().init(this);
}

void MetalRenderer::shutdown()
{
	sharedMaterialLibrary().shutdown();
	sharedConstBuffer().shutdown();
	sharedVariables().shutdown();

	ET_OBJC_RELEASE(_private->metal.queue);
	ET_OBJC_RELEASE(_private->metal.device);
}

void MetalRenderer::begin()
{
	MetalState& mtl = _private->metal;

	mtl.mainDrawable = [mtl.layer nextDrawable];
	ET_ASSERT(mtl.mainDrawable != nil);

	id<MTLTexture> mainTexture = mtl.mainDrawable.texture;

	if ((mtl.defaultDepthBuffer == nil) ||
		(mainTexture.width != mtl.defaultDepthBuffer.width) ||
		(mainTexture.height != mtl.defaultDepthBuffer.height))
	{
		MTLTextureDescriptor* depthDesc =
			[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
			width:mainTexture.width height:mainTexture.height mipmapped:NO];
		depthDesc.usage = MTLTextureUsageRenderTarget;
		depthDesc.storageMode = MTLStorageModePrivate;
		mtl.defaultDepthBuffer = [mtl.device newTextureWithDescriptor:depthDesc];
	}

	mtl.mainCommandBuffer = [mtl.queue commandBuffer];
	ET_ASSERT(mtl.mainCommandBuffer != nil);

	sharedConstBuffer().reset();
}

void MetalRenderer::present()
{
	[_private->metal.mainCommandBuffer presentDrawable:_private->metal.mainDrawable];
	[_private->metal.mainCommandBuffer commit];
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
	
}

/*
 * Buffers
 */
DataBuffer::Pointer MetalRenderer::createDataBuffer(const std::string& name, uint32_t size)
{
	return MetalDataBuffer::Pointer::create(_private->metal, size);
}

DataBuffer::Pointer MetalRenderer::createDataBuffer(const std::string& name, const BinaryDataStorage& data)
{
	return MetalDataBuffer::Pointer::create(_private->metal, data);
}

VertexBuffer::Pointer MetalRenderer::createVertexBuffer(const std::string& name, VertexStorage::Pointer vs, BufferDrawType dt)
{
	return MetalVertexBuffer::Pointer::create(_private->metal, vs->declaration(), vs->data(), dt, name);
}

IndexBuffer::Pointer MetalRenderer::createIndexBuffer(const std::string& name, IndexArray::Pointer ia, BufferDrawType dt)
{
    return MetalIndexBuffer::Pointer::create(_private->metal, ia, dt, name);
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
Program::Pointer MetalRenderer::createProgram(const std::string& source, const std::string&)
{
    MetalProgram::Pointer program = MetalProgram::Pointer::create(_private->metal);
	program->build(source);
    return program;
}

/*
 * Pipeline state
 */
PipelineState::Pointer MetalRenderer::acquirePipelineState(RenderPass::Pointer pass, Material::Pointer mtl,
    VertexStream::Pointer vs)
{
	PipelineState::Pointer result = _private->cache.find(vs->vertexBuffer()->declaration(),
		mtl->program(), mtl->depthState(), mtl->blendState(), mtl->cullMode(), TextureFormat::RGBA8,
		vs->indexBuffer()->primitiveType());

	if (result.invalid())
	{
		result = MetalPipelineState::Pointer::create(this, _private->metal);
		result->setProgram(mtl->program());
		result->setCullMode(mtl->cullMode());
		result->setDepthState(mtl->depthState());
		result->setBlendState(mtl->blendState());
		result->setRenderTargetFormat(TextureFormat::RGBA8);
		result->setInputLayout(vs->vertexBuffer()->declaration());
		result->setPrimitiveType(vs->indexBuffer()->primitiveType());
		result->build();
		
		_private->cache.addToCache(result);
	}

	return result;
}

/*
 * Sampler
 */
Sampler::Pointer MetalRenderer::createSampler(const Sampler::Description& desc)
{
	auto i = _private->samplersCache.find(desc.hash);
	if (i != _private->samplersCache.end())
		return i->second;

	MetalSampler::Pointer smp = MetalSampler::Pointer::create(_private->metal, desc);
	_private->samplersCache.emplace(desc.hash, smp);
	return smp;
}

}
