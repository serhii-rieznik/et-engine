/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/dx12/dx12_buffer.h>
#include <et/rendering/dx12/dx12.h>

namespace et
{
class DX12BufferPrivate : public DX12NativeBuffer
{
public:
	DX12BufferPrivate(DX12& dx) : dx12(dx) { }

	DX12& dx12;
	Buffer::Description desc;
	D3D12_RANGE modifiedRange { };
	bool mapped = false;
};

DX12Buffer::DX12Buffer(DX12& dx12, const Description& desc)
{
	ET_PIMPL_INIT(DX12Buffer, dx12);

	_private->desc.location = desc.location;
	_private->desc.usage = desc.usage;
	_private->desc.size = desc.size;
	_private->desc.alignedSize = alignUpTo(desc.size, 256u);

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = (_private->desc.location == Buffer::Location::Device) ? D3D12_HEAP_TYPE_DEFAULT : D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Width = _private->desc.alignedSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	switch (_private->desc.usage)
	{
	case Buffer::Usage::Vertex:
	{
		_private->targetState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		break;
	}
	case Buffer::Usage::Index:
	{
		_private->targetState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		break;
	}
	case Buffer::Usage::Constant:
	case Buffer::Usage::Staging:
	{
		_private->targetState = D3D12_RESOURCE_STATE_GENERIC_READ;
		break;
	}
	default:
		ET_ASSERT("Invalid Buffer::Usage provided");
	}
	_private->currentState = D3D12_RESOURCE_STATE_GENERIC_READ;
	
	DX12_CALL(_private->dx12.device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, 
		_private->currentState, nullptr, IID_PPV_ARGS(_private->buffer.GetAddressOf())));

	if (desc.initialData.size() > 0)
		updateData(0, desc.initialData);
}

DX12Buffer::~DX12Buffer()
{
	ET_PIMPL_FINALIZE(DX12Buffer);
}

uint8_t* DX12Buffer::map(uint32_t begin, uint32_t length)
{
	_private->modifiedRange = { };

	void* result = nullptr;
	D3D12_RANGE range = { begin, length };
	DX12_CALL(_private->buffer->Map(0, &range, &result));
	
	_private->mapped = true;
	return reinterpret_cast<uint8_t*>(result);
}

void DX12Buffer::modifyRange(uint64_t begin, uint64_t length)
{
	_private->modifiedRange.Begin = std::min(_private->modifiedRange.Begin, begin);
	_private->modifiedRange.End = std::max(_private->modifiedRange.End, begin + length);
}

void DX12Buffer::unmap()
{
	_private->buffer->Unmap(0, &_private->modifiedRange);
	_private->mapped = false;
}

bool DX12Buffer::mapped() const
{
	return _private->mapped;
}

void DX12Buffer::updateData(uint32_t offset, const BinaryDataStorage& data)
{
	retain();

	if (_private->desc.location == Buffer::Location::Device)
	{
		Buffer::Description desc;
		desc.initialData = BinaryDataStorage(data.data(), data.size());
		desc.usage = Buffer::Usage::Staging;
		desc.size = data.size();
		desc.location = Buffer::Location::Host;
		
		DX12Buffer uploadBuffer(_private->dx12, desc);
		uploadBuffer.transferData(Buffer::Pointer(this));
	}
	else 
	{
		uint8_t* mappedData = map(offset, data.size());
		memcpy(mappedData, data.data(), data.size());
		modifyRange(0, data.size());
		unmap();
	}

	release();
}

void DX12Buffer::transferData(Buffer::Pointer destination)
{
	DX12Buffer::Pointer dst = destination;
	_private->dx12.executeServiceCommands([this, dst](ID3D12GraphicsCommandList* cmdList){

		Vector<D3D12_RESOURCE_BARRIER> barriersFrom;
		barriersFrom.reserve(2);

		if (_private->currentState != D3D12_RESOURCE_STATE_GENERIC_READ)
		{
			barriersFrom.emplace_back();
			barriersFrom.back().Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barriersFrom.back().Transition.pResource = _private->buffer.Get();
			barriersFrom.back().Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barriersFrom.back().Transition.StateBefore = _private->currentState;
			barriersFrom.back().Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		if (dst->_private->currentState != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			barriersFrom.emplace_back();
			barriersFrom.back().Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barriersFrom.back().Transition.pResource = dst->_private->buffer.Get();
			barriersFrom.back().Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barriersFrom.back().Transition.StateBefore = dst->_private->currentState;
			barriersFrom.back().Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		}

		Vector<D3D12_RESOURCE_BARRIER> barriersTo;
		barriersTo.reserve(2);

		if (_private->targetState != D3D12_RESOURCE_STATE_GENERIC_READ)
		{
			barriersTo.emplace_back();
			barriersTo.back().Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barriersTo.back().Transition.pResource = _private->buffer.Get();
			barriersTo.back().Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barriersTo.back().Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
			barriersTo.back().Transition.StateAfter = _private->targetState;
			_private->currentState = _private->targetState;
		}
		if (dst->_private->targetState != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			barriersTo.emplace_back();
			barriersTo.back().Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barriersTo.back().Transition.pResource = dst->_private->buffer.Get();
			barriersTo.back().Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barriersTo.back().Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barriersTo.back().Transition.StateAfter = dst->_private->targetState;
			dst->_private->currentState = dst->_private->targetState;
		}

		cmdList->ResourceBarrier(static_cast<UINT>(barriersFrom.size()), barriersFrom.data());
		cmdList->CopyResource(dst->_private->buffer.Get(), _private->buffer.Get());
		cmdList->ResourceBarrier(static_cast<UINT>(barriersTo.size()), barriersTo.data());
	});
}

uint32_t DX12Buffer::size() const
{
	return _private->desc.size;
}

}

