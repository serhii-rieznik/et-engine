/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_renderpass.h>

namespace et
{

DX12RenderPass::DX12RenderPass(const RenderPass::ConstructionInfo& info) 
	: RenderPass(nullptr, info)
{
}

void DX12RenderPass::begin()
{
}

void DX12RenderPass::validateRenderBatch(RenderBatch::Pointer)
{

}

void DX12RenderPass::pushRenderBatch(RenderBatch::Pointer)
{
}

void DX12RenderPass::end()
{
}

}
