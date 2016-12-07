/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12.h>
#include <comdef.h>

namespace et
{
namespace dx12
{

std::string resultToString(HRESULT hr)
{
	char buffer[1024] = {};
	uint32_t offset = sprintf(buffer, "HRESULT 0x%08X: ", hr);
	try
	{
		DWORD ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			buffer + offset, sizeof(buffer) - 1 - offset, nullptr);
	}
	catch (...)
	{
	}
	return buffer;
}

}

void DX12::executeServiceCommands(ServiceCommands commands)
{
	ComPtr<ID3D12GraphicsCommandList> commandList;

	DX12_CALL(device->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(),
		nullptr, IID_PPV_ARGS(commandList.GetAddressOf())));
	commands(commandList.Get());
	commandList->Close();
	
	ID3D12CommandList* execute[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, execute);
	
	waitForExecution();
}

void DX12::waitForExecution()
{
	UINT64 signalValue = fenceValue++;
	DX12_CALL(commandQueue->Signal(fence.Get(), signalValue));
	if (fence->GetCompletedValue() < fenceValue)
	{
		DX12_CALL(fence->SetEventOnCompletion(signalValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

}
