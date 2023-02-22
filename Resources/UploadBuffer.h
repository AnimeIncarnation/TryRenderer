#pragma once
#ifndef _UPLOADBUFFER_H_
#define _UPLOADBUFFER_H_

#include "Buffer.h"
#include <span>

class UploadBuffer final :public Buffer
{
private:
	ComPtr<ID3D12Resource> resource;
	UINT64 size;

public:
	UploadBuffer(DXDevice* device, UINT64 size);

	ID3D12Resource* GetResource() const;
	void CopyData(uint64 offset, std::span<byte const> data);//全用UploadBuffer来copy数据
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const override { return resource->GetGPUVirtualAddress(); }
	UINT64 GetSize() const override { return size; }

};

#endif