#include "DefaultBuffer.h"
#include "../DXSampleHelper.h"

DefaultBuffer::DefaultBuffer(
	DXDevice* device,
	UINT64 size,
	D3D12_RESOURCE_STATES initState):Buffer(device),size(size),initState(initState)
{
    auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto buffer = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    ThrowIfFailed(device->Device()->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &buffer,
        initState,
        nullptr,
        IID_PPV_ARGS(&resource)));
}
