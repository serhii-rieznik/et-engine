/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_renderpass.h>
#include <et/rendering/dx12/dx12.h>

namespace et
{

DX12RenderPass::DX12RenderPass(RenderInterface* renderer, DX12& dx12, const RenderPass::ConstructionInfo& info) 
	: RenderPass(renderer, info)
{
}

void DX12RenderPass::begin()
{
}

void DX12RenderPass::pushRenderBatch(const RenderBatch::Pointer&)
{
}

void DX12RenderPass::end()
{
}

}
