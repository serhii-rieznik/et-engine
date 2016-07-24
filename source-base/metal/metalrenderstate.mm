/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.h>
#include <et/metal/metalrenderstate.h>

#if (ET_PLATFORM_MAC)
#	include <Metal/Metal.h>
#	include <MetalKit/MetalKit.h>
#else
#	error Not implemented for this platform
#endif

namespace et
{

void MetalRenderState::setBlendState(const BlendState&)
{
	
}

void MetalRenderState::setDepthState(const DepthState&)
{
	
}

void MetalRenderState::setCullMode(const CullMode&)
{

}

}
