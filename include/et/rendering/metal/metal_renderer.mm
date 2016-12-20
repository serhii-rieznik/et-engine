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
#include <et/rendering/metal/metal_buffer.h>
#include <et/rendering/metal/metal_texture.h>
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
    _private->metal.layer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];

	application().context().objects[3] = (__bridge void*)(_private->metal.device);
	application().context().objects[4] = (__bridge void*)_private->metal.layer;

	sharedConstantBuffer().init(this);
	sharedMaterialLibrary().init(this);
}

void MetalRenderer::shutdown()
{
	sharedMaterialLibrary().shutdown();
	sharedConstantBuffer().shutdown();

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

void MetalRenderer::submitRenderPass(RenderPass::Pointer)
{
	
}

/*
 * Buffers
 */
Buffer::Pointer MetalRenderer::createBuffer(const std::string& name, const Buffer::Description& desc)
{
	return MetalBuffer::Pointer::create(_private->metal, desc);
}

/*
 * Textures
 */
Texture::Pointer MetalRenderer::createTexture(TextureDescription::Pointer desc)
{
    return MetalTexture::Pointer::create(_private->metal, desc);
}

TextureSet::Pointer MetalRenderer::createTextureSet(const TextureSet::Description&)
{
	return TextureSet::Pointer();
}

/*
 * Programs
 */
Program::Pointer MetalRenderer::createProgram(const std::string& source)
{
    MetalProgram::Pointer program = MetalProgram::Pointer::create(_private->metal);
	program->build(source);
    return program;
}

/*
 * Pipeline state
 */
PipelineState::Pointer MetalRenderer::acquirePipelineState(const RenderPass::Pointer&,
	const Material::Pointer&, const VertexStream::Pointer&)
{
	return PipelineState::Pointer();
	/*
	 * TODO : rewrite
	 *
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
	// */
}

/*
 * Sampler
 */
Sampler::Pointer MetalRenderer::createSampler(const Sampler::Description& desc)
{
	return MetalSampler::Pointer::create(_private->metal, desc);
}

void MetalRenderer::resize(const vec2i&)
{
	ET_FAIL("Not implemented");
}

}
