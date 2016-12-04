/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

namespace et
{

using namespace Microsoft::WRL;

struct DX12
{
	ComPtr<IDXGIFactory2> factory;
	ComPtr<IDXGIAdapter> adapter;

	ComPtr<ID3D12Device> device;
};

}
