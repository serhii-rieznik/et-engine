/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_buffer.h>
#include <et/rendering/dx12/dx12_program.h>
#include <et/rendering/dx12/dx12_texture.h>
#include <et/rendering/dx12/dx12_renderpass.h>
#include <et/rendering/dx12/dx12_pipelinestate.h>
#include <et/rendering/dx12/dx12_renderer.h>
#include <et/rendering/dx12/dx12.h>
#include <et/app/application.h>

namespace et
{

class DX12RendererPrivate : public DX12
{
public:
	void enumAdapters();
	DX12& dx12() { return *this; }
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
#if (ET_DEBUG)
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(_private->debug.GetAddressOf()));
	if (SUCCEEDED(result))
	{
		_private->debug->EnableDebugLayer();
	}
#endif

	_private->enumAdapters();
	DX12_CALL(D3D12CreateDevice(_private->adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(_private->device.GetAddressOf())));
	DX12_CALL(_private->device->CreateFence(_private->fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_private->fence.GetAddressOf())));
	_private->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	DX12_CALL(_private->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(_private->commandQueue.GetAddressOf())));
	DX12_CALL(_private->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_private->commandAllocator.GetAddressOf())));

	RECT clientRect = { };
	HWND mainWindow = reinterpret_cast<HWND>(application().context().objects[0]);
	GetClientRect(mainWindow, &clientRect);

	DXGI_SWAP_CHAIN_DESC1 desc = { };
	desc.Width = clientRect.right - clientRect.left;
	desc.Height = clientRect.bottom - clientRect.top;
	desc.Format = DX12::colorFormat();
	desc.BufferCount = DX12::ColorBuffersCount;
	desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SampleDesc.Count = 1;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	ComPtr<IDXGISwapChain1> swapChain1;
	DX12_CALL(_private->factory->CreateSwapChainForHwnd(_private->commandQueue.Get(), mainWindow, &desc, 
		nullptr, nullptr, swapChain1.GetAddressOf()));
	DX12_CALL(swapChain1.As(&_private->swapChain));
	DX12_CALL(_private->factory->MakeWindowAssociation(mainWindow, DXGI_MWA_NO_ALT_ENTER));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = DX12::ColorBuffersCount;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	DX12_CALL(_private->device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_private->rtvDescriptorHeap.GetAddressOf())));
	_private->rtvDescriptorHeapIncrementSize = _private->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_private->rtvFirstHandle = _private->rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DX12_CALL(_private->device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_private->dsvDescriptorHeap.GetAddressOf())));
	_private->dsvDescriptorHeapIncrementSize = _private->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_private->depthView = _private->dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	resize(vec2i(desc.Width, desc.Height));

	initInternalStructures();
}

void DX12Renderer::shutdown()
{
	shutdownInternalStructures();
	CloseHandle(_private->fenceEvent);
}

void DX12Renderer::resize(const vec2i& size)
{
	for (UINT i = 0; i < DX12::ColorBuffersCount; ++i)
		_private->colorBuffers[i].resource.Reset();
	_private->depthBuffer.Reset();

	DX12_CALL(_private->swapChain->ResizeBuffers(DX12::ColorBuffersCount, size.x, size.y, DX12::colorFormat(), 0));
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _private->rtvFirstHandle;
	for (UINT i = 0; i < DX12::ColorBuffersCount; ++i)
	{
		DX12_CALL(_private->swapChain->GetBuffer(i, IID_PPV_ARGS(_private->colorBuffers[i].resource.GetAddressOf())));
		_private->device->CreateRenderTargetView(_private->colorBuffers[i].resource.Get(), nullptr, rtvHandle);
		_private->colorBuffers[i].view = rtvHandle;
		rtvHandle.ptr += _private->rtvDescriptorHeapIncrementSize;
	}

	D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_DEFAULT };
	
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DX12::depthFormat();

	D3D12_RESOURCE_DESC depthBufferDesc = {};
	depthBufferDesc.Width = size.x;
	depthBufferDesc.Height = size.y;
	depthBufferDesc.Format = DX12::depthFormat();
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.DepthOrArraySize = 1;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	DX12_CALL(_private->device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthBufferDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(_private->depthBuffer.GetAddressOf())));
	_private->device->CreateDepthStencilView(_private->depthBuffer.Get(), nullptr, _private->depthView);
}

void DX12Renderer::begin()
{
	_private->currentFrameIndex = _private->swapChain->GetCurrentBackBufferIndex();
}

void DX12Renderer::present()
{
	DX12_CALL(_private->swapChain->Present(1, 0));
	_private->waitForExecution();
}

Buffer::Pointer DX12Renderer::createBuffer(const Buffer::Description& desc)
{
	return DX12Buffer::Pointer::create(_private->dx12(), desc);
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
	return DX12RenderPass::Pointer::create(this, _private->dx12(), info);
}

void DX12Renderer::submitRenderPass(RenderPass::Pointer)
{
}

/* 
 * Private implementation
 */
void DX12RendererPrivate::enumAdapters()
{
#if (ET_DEBUG)
	HRESULT result = DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiDebug.GetAddressOf()));
	if (SUCCEEDED(result))
	{
		DX12_CALL(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(factory.GetAddressOf())));
	}
	else 
#endif
	{
		DX12_CALL(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.GetAddressOf())));
	}

	using AdapterInfo = std::pair<ComPtr<IDXGIAdapter1>, DXGI_ADAPTER_DESC1>;
	Vector<AdapterInfo> adapters;
	UINT adapterIndex = 0;
	ComPtr<IDXGIAdapter1> localAdapter;
	while (factory->EnumAdapters1(adapterIndex, localAdapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc = {};
		DX12_CALL(localAdapter->GetDesc1(&desc));
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
