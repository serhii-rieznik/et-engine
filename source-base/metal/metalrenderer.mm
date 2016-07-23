/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/metal/metalrenderer.h>

#if (ET_PLATFORM_MAC)
#	include <Metal/Metal.h>
#	include <MetalKit/MetalKit.h>
#else
#	error Not implemented for this platform
#endif

namespace et
{

class MetalRendererPrivate
{
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
}

void MetalRenderer::shutdown()
{
}

void MetalRenderer::begin()
{
}

void MetalRenderer::present()
{
}

void MetalRenderer::drawIndexedPrimitive(PrimitiveType pt, IndexArrayFormat fmt, uint32_t first, uint32_t count)
{
}

RenderPass::Pointer MetalRenderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	RenderPass::Pointer result = RenderPass::Pointer::create(info);
	return result;
}

void MetalRenderer::submitRenderPass(RenderPass::Pointer pass)
{
}


}
