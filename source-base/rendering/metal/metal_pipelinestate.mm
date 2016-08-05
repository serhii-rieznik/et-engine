/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/metal/metal_pipelinestate.h>
#include <et/rendering/metal/metal.h>

namespace et
{

class MetalPipelineStatePrivate
{
public:
	MetalPipelineStatePrivate(MetalState& mtl) :
		metal(mtl) { }

	MetalState& metal;
	id<MTLRenderPipelineState> pipelineState = nil;
};

MetalPipelineState::MetalPipelineState(MetalState& mtl)
{
	ET_PIMPL_INIT(MetalPipelineState, mtl);
}

MetalPipelineState::~MetalPipelineState()
{
	ET_OBJC_RELEASE(_private->pipelineState);
}

void MetalPipelineState::build()
{

}

}
