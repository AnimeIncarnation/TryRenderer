#include "UploadBuffer.h"
#include "../DXSampleHelper.h"
#include "../stdafx.h"
#include <iostream>

UploadBuffer::UploadBuffer(DXDevice* device, UINT64 size):Buffer(device),size(size)
{
    ThrowIfFailed(device->Device()->CreateCommittedResource(
        get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),//ʹ��CreateCommittedResource�Զ�����һ���Ѳ���
        D3D12_HEAP_FLAG_NONE,
        get_rvalue_ptr(CD3DX12_RESOURCE_DESC::Buffer(size)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource)));
}

void UploadBuffer::CopyData(uint64 offset, std::span<const byte> data)
{
    void* mappedCPUPtr;
    //D3D12_RANGE range{offset, min(size, offset + data.size())};    //range����һ������begin��end�Ľṹ
    //ThrowIfFailed(resource->Map(0, &range, reinterpret_cast<void**>(&mappedCPUPtr)));
    //memcpy(reinterpret_cast<byte*>(mappedCPUPtr) + offset, data.data(), range.End - range.Begin);
    //resource->Unmap(0, &range);
    // 
    //nullptr��ʾȫ����Χ
    ThrowIfFailed(resource->Map(0, nullptr, reinterpret_cast<void**>(&mappedCPUPtr)));
    memcpy(reinterpret_cast<byte*>(mappedCPUPtr) + offset, data.data(), data.size());
    //memcpy(&mappedCPUPtr[data.size()], data.data(), data.size());
    resource->Unmap(0, nullptr);
}

ID3D12Resource* UploadBuffer::GetResource() const
{
    return resource.Get(); 
}