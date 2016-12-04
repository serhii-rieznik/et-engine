/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_program.h>
#include <et/rendering/dx12/dx12_texture.h>
#include <et/rendering/dx12/dx12_renderpass.h>
#include <et/rendering/dx12/dx12_pipelinestate.h>
#include <et/rendering/dx12/dx12_renderer.h>
#include <et/rendering/dx12/dx12.h>

namespace et
{

class DX12RendererPrivate : public DX12
{
public:
	void enumAdapters();
};

DX12Renderer::DX12Renderer(RenderContext * rc) 
	: RenderInterface(rc)
{
	ET_PIMPL_INIT(DX12Renderer);
}

DX12Renderer::~DX12Renderer()
{
	ET_PIMPL_FINALIZE(DX12Renderer);
}

void DX12Renderer::init(const RenderContextParameters& params)
{
	_private->enumAdapters();
	HRESULT result = D3D12CreateDevice(_private->adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(_private->device.GetAddressOf()));

	initInternalStructures();
}

void DX12Renderer::shutdown()
{
	shutdownInternalStructures();
}

void DX12Renderer::resize(const vec2i &)
{
}

void DX12Renderer::begin()
{

}

void DX12Renderer::present()
{
}

Buffer::Pointer DX12Renderer::createBuffer(const Buffer::Description&)
{
	return Buffer::Pointer();
}

Texture::Pointer DX12Renderer::createTexture(TextureDescription::Pointer desc)
{
	return DX12Texture::Pointer::create(desc.reference(), desc->data);
}

TextureSet::Pointer DX12Renderer::createTextureSet(const TextureSet::Description&)
{
	return TextureSet::Pointer();
}

Sampler::Pointer DX12Renderer::createSampler(const Sampler::Description &)
{
	return Sampler::Pointer();
}

PipelineState::Pointer DX12Renderer::acquirePipelineState(const RenderPass::Pointer&, const Material::Pointer&, const VertexStream::Pointer&)
{
	return PipelineState::Pointer();
}

Program::Pointer DX12Renderer::createProgram(const std::string&)
{
	return DX12Program::Pointer::create();
}

RenderPass::Pointer DX12Renderer::allocateRenderPass(const RenderPass::ConstructionInfo& info)
{
	return DX12RenderPass::Pointer::create(info);
}

void DX12Renderer::submitRenderPass(RenderPass::Pointer)
{
}

/* 
 * Private implementation
 */
void DX12RendererPrivate::enumAdapters()
{
	UINT factoryFlags = 0;
	HRESULT result = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(factory.GetAddressOf()));

	using AdapterInfo = std::pair<ComPtr<IDXGIAdapter1>, DXGI_ADAPTER_DESC1>;
	Vector<AdapterInfo> adapters;
	UINT adapterIndex = 0;
	ComPtr<IDXGIAdapter1> localAdapter;
	while (factory->EnumAdapters1(adapterIndex, localAdapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc = {};
		localAdapter->GetDesc1(&desc);
		if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
		{
			adapters.emplace_back(localAdapter, desc);
		}
		++adapterIndex;
	}

	for (const auto& a : adapters)
	{
		// TODO : select preferred
		adapter = a.first;
		break;
	}
}

}
