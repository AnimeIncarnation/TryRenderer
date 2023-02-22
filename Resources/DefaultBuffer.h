#pragma once
#ifndef _DEFAULTBUFFER_H_
#define _DEFAULTBUFFER_H_

#include "../d3dx12.h"
#include "Buffer.h"

//允许移动，不允许拷贝
class DefaultBuffer final : public Buffer
{
private:
	UINT64 size;
	D3D12_RESOURCE_STATES initState;
	ComPtr<ID3D12Resource> resource;

public:
	DefaultBuffer(
		DXDevice* device,
		UINT64 byteSize,
		D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON);
	DefaultBuffer(DefaultBuffer&&) = default;
	DefaultBuffer(const DefaultBuffer &) = delete;
	DefaultBuffer& operator=(const DefaultBuffer &) = delete;
	//~DefaultBuffer() = default;

public:
	D3D12_RESOURCE_STATES GetResourceState() const override { return initState; }
	UINT64 GetSize() const override { return size; }
	ID3D12Resource* GetResource() const override { return resource.Get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const override { return resource->GetGPUVirtualAddress(); }

};


#endif