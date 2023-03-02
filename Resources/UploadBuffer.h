#pragma once
#ifndef _UPLOADBUFFER_H_
#define _UPLOADBUFFER_H_

#include "Buffer.h"
#include <span>

class UploadBuffer final :public Buffer
{
	ComPtr<ID3D12Resource> resource;
	UINT64 size;

public:
	UploadBuffer(DXDevice* device, UINT64 size);
	UploadBuffer(const UploadBuffer&) = delete;
	UploadBuffer& operator=(const UploadBuffer&) = delete;
	UploadBuffer(UploadBuffer&&) = default;

	ID3D12Resource* GetResource() const;
	void CopyData(uint64 offset, std::span<byte const> data);//ȫ��UploadBuffer��copy����
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const override { return resource->GetGPUVirtualAddress(); }
	UINT64 GetSize() const override { return size; }

};

#endif