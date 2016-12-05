/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DXGIDebug.h>
#include <wrl.h>

namespace et
{

using namespace Microsoft::WRL;

struct DX12
{
public:
	enum : UINT
	{
		ColorBuffersCount = 3
	};

	static DXGI_FORMAT colorFormat() { return DXGI_FORMAT_R8G8B8A8_UNORM; }
	static DXGI_FORMAT depthFormat() { return DXGI_FORMAT_D32_FLOAT; }

public:
	ComPtr<IDXGIFactory2> factory;
	ComPtr<IDXGIAdapter> adapter;
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<IDXGIDebug1> dxgiDebug;

	ComPtr<ID3D12Debug> debug;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> commandQueue;

	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvFirstHandle{};
	UINT rtvDescriptorHeapIncrementSize = 0;

	struct ColorBuffer
	{
		ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE view{};
	}  colorBuffers[ColorBuffersCount];

	ComPtr<ID3D12Resource> depthBuffer;
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE depthView{};
	UINT dsvDescriptorHeapIncrementSize = 0;

	UINT currentFrameIndex = 0;
};

namespace dx12
{
std::string resultToString(HRESULT);
}

#define DX12_CALL(expr) do { \
	HRESULT hr = (expr); \
	if (FAILED(hr)) { \
		et::log::error("DirectX call failed: %s\nat %s [%d]\n%s", \
			(#expr), __FILE__, __LINE__, dx12::resultToString(hr).c_str());\
	et::debug::debugBreak();\
	} } while (0)

}
