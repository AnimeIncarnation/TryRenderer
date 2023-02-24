#include "FrameResource.h"


FrameResource::FrameResource(DXDevice* device, UINT cbufferSize)
{
	ThrowIfFailed(device->Device()->
		CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdAllocator.GetAddressOf())));
	ThrowIfFailed(device->Device()->
		CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(cmdList.GetAddressOf())));
	ThrowIfFailed(cmdList->Close());

	constantUpload = std::make_unique<UploadBuffer>(device, cbufferSize);
	constantDefault = std::make_unique<DefaultBuffer>(device, cbufferSize);

}


//void FrameResource::AddDelayDisposeResource(ComPtr<ID3D12Resource> const& ptr)
//{
//	delayDisposeResources.emplace_back(ptr);
//}

//Execute，Signal，Sync的if(!populated) return;都是为了处理初始情况。注意Sync的||fenceID==0使得前三帧完全不等待，
void FrameResource::Execute(ID3D12CommandQueue* queue, UINT64& newFence)
{
	if (!populated) return;
	fenceID = ++newFence;
	ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
	queue->ExecuteCommandLists(array_count(ppCommandLists),ppCommandLists);

}

void FrameResource::Signal(ID3D12CommandQueue* queue, ID3D12Fence* fence)
{
	if (!populated) return;
	queue->Signal(fence, fenceID);
}

void FrameResource::Sync(ID3D12Fence* fence)
{
	if (!populated || fenceID == 0) return;
	if (fence->GetCompletedValue() < fenceID)
	{
		LPCWSTR falseValue = 0;
		HANDLE eventHandle = CreateEventEx(nullptr, falseValue, false, EVENT_ALL_ACCESS);
		//等待m_fence->GetCompletedValue()值变为fence值时，执行m_fenceEvent
		ThrowIfFailed(fence->SetEventOnCompletion(fenceID, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
	//delayDisposeResources.clear();
}

//
//CommandListHandle FrameResource::Command()
//{
//	return {cmdAllocator.Get(), cmdList.Get()};
//}

void FrameResource::Populate()
{
	populated = true;
}