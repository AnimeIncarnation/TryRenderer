#include "FrameResource.h"


FrameResource::FrameResource(DXDevice* device)
{
	ThrowIfFailed(device->Device()->
		CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdAllocator.GetAddressOf())));
	ThrowIfFailed(device->Device()->
		CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(cmdList.GetAddressOf())));
	ThrowIfFailed(cmdList->Close());
}

void FrameResource::EmplaceConstantBuffer(DXDevice* device, UINT cbufferSize)
{
	constantUpload.emplace_back(std::make_unique<UploadBuffer>(device, cbufferSize));
	constantDefault.emplace_back(std::make_unique<DefaultBuffer>(device, cbufferSize));
	constantBufferSize.emplace_back(cbufferSize);
	mappedCbvData.emplace_back(nullptr);
}

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
}

//设置RS和OM
void FrameResource::SetRenderTarget(CD3DX12_VIEWPORT const* viewport, CD3DX12_RECT const* scissorRect, CD3DX12_CPU_DESCRIPTOR_HANDLE const* rtvHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE const* dsvHandle)
{
	cmdList->RSSetViewports(1, viewport);
	cmdList->RSSetScissorRects(1, scissorRect);
	cmdList->OMSetRenderTargets(1, rtvHandle, FALSE, dsvHandle);
}

//Clear1
void FrameResource::ClearRenderTarget(const CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle)
{
	const float clearColor[] = { 0.0f, 0.2f, 0.3f, 1.0f };	   //0231
	cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

//Clear2
void FrameResource::ClearDepthStencilBuffer(const CD3DX12_CPU_DESCRIPTOR_HANDLE& dsvHandle)
{
	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
}

//设置IA，设置PSO，Draw
void FrameResource::DrawMesh(DXDevice* device, Model* model, ID3D12PipelineState* pipelineState)
{
	cmdList->SetPipelineState(pipelineState);
	for (int i = 0; i < model->VertexBuffers().size();i++)
	{
		cmdList->IASetVertexBuffers(0, 1, &model->GetVertexBufferView()[i]);
		cmdList->IASetIndexBuffer(&model->GetIndexBufferView()[i]);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);      //更细化的图元拓扑：列表/条带......
		cmdList->DrawIndexedInstanced(model->GetIndiceCount()[i], 1, 0, 0, 0);
		//cmdList->DispatchMesh(1, 1, 1);
	}
}

void FrameResource::DrawMeshlet(DXDevice* device, Model* model, ID3D12PipelineState* pipelineState, RasterShader* shader,UINT instanceCount)
{
	cmdList->SetPipelineState(pipelineState);
	//每个Mesh绑定自己的UAR
	for (int i = 0; i < model->VertexBuffers().size();i++)
	//for (int i = 0; i < 1 ;i++)
	{
		D3D12_GPU_VIRTUAL_ADDRESS meshInfo = model->GetMeshletInfoAddress(i);
		D3D12_GPU_VIRTUAL_ADDRESS meshletAddress = model->MeshletBuffers()[i].GetGPUAddress();
		D3D12_GPU_VIRTUAL_ADDRESS vertexAddress = model->VertexBuffers()[i].GetGPUAddress();
		D3D12_GPU_VIRTUAL_ADDRESS vertexIndiceAddress = model->VertexIndiceBuffers()[i].GetGPUAddress();
		D3D12_GPU_VIRTUAL_ADDRESS primitiveIndiceAddress = model->PrimitiveIndiceBuffers()[i].GetGPUAddress();

		shader->SetParameter(cmdList.Get(), "MeshInfo", meshInfo);
		shader->SetParameter(cmdList.Get(), "Meshlets", meshletAddress);
		shader->SetParameter(cmdList.Get(), "Vertices", vertexAddress);
		shader->SetParameter(cmdList.Get(), "VertexIndices", vertexIndiceAddress);
		shader->SetParameter(cmdList.Get(), "PrimitiveIndices", primitiveIndiceAddress);

		//没有AS的时候，DispatchMesh数应等于meshletCountInThisMesh * InstanceCount
		//有AS的时候，如下
		UINT meshletCountInThisMesh = model->GetMeshletCount()[i];
		UINT ASThreadCountInOneGroup = 32;
		UINT ASGroups = (meshletCountInThisMesh * instanceCount + ASThreadCountInOneGroup - 1) / ASThreadCountInOneGroup;
		cmdList->DispatchMesh(ASGroups, 1, 1);
	}
}

void FrameResource::CopyConstantFromUploadToDefault()
{
	for (int i = 0;i < constantUpload.size();i++)
	{
		//cmdList->ResourceBarrier(1,
		//	get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
		//		constantDefault[i]->GetResource(),
		//		D3D12_RESOURCE_STATE_COMMON,
		//		D3D12_RESOURCE_STATE_COPY_DEST)));
		cmdList->CopyBufferRegion(constantDefault[i]->GetResource(), 0, constantUpload[i]->GetResource(), 0, constantBufferSize[i]);
		cmdList->ResourceBarrier(1,
			get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
				constantDefault[i]->GetResource(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_COMMON)));
	}
	
}

void FrameResource::Populate()
{
	populated = true;
}