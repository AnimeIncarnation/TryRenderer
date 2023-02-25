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

//Execute��Signal��Sync��if(!populated) return;����Ϊ�˴����ʼ�����ע��Sync��||fenceID==0ʹ��ǰ��֡��ȫ���ȴ���
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
		//�ȴ�m_fence->GetCompletedValue()ֵ��Ϊfenceֵʱ��ִ��m_fenceEvent
		ThrowIfFailed(fence->SetEventOnCompletion(fenceID, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

//����RS��OM
void FrameResource::SetRenderTarget(CD3DX12_VIEWPORT const* viewport, CD3DX12_RECT const* scissorRect, CD3DX12_CPU_DESCRIPTOR_HANDLE const* rtvHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE const* dsvHandle)
{
	cmdList->RSSetViewports(1, viewport);
	cmdList->RSSetScissorRects(1, scissorRect);
	cmdList->OMSetRenderTargets(1, rtvHandle, FALSE, dsvHandle);
}

//Clear1
void FrameResource::ClearRenderTarget(const CD3DX12_CPU_DESCRIPTOR_HANDLE& rtvHandle)
{
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

//Clear2
void FrameResource::ClearDepthStencilBuffer(const CD3DX12_CPU_DESCRIPTOR_HANDLE& dsvHandle)
{
	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
}

//����IA��Shader������PSO�������յ�Draw��������Asset�׶�û�д���PSO������Ҫ����PSO��
void FrameResource::DrawMesh(DXDevice* device, const RasterShader* shader, PSOManager* psoManager, Mesh* mesh, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat)
{

	ID3D12PipelineState * pipelineState = psoManager->GetPipelineState( //ʹ��PSOManager������PipelineState
		device,
		*shader,
		mesh->Layout(),
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		{ &colorFormat,1 },	   //span�Ĺ��죺�׵�ַ + ����
		depthFormat);
	cmdList->SetPipelineState(pipelineState);

	std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;
	mesh->GetVertexBufferView(vertexBufferView);
	D3D12_INDEX_BUFFER_VIEW indexBufferView = mesh->GetIndexBufferView();
	cmdList->IASetVertexBuffers(0, vertexBufferView.size(), vertexBufferView.data());
	cmdList->IASetIndexBuffer(&indexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);      //��ϸ����ͼԪ���ˣ��б�/����......
	cmdList->DrawIndexedInstanced(mesh->GetIndiceCount(), 1, 0, 0, 0);

	
}


void FrameResource::Populate()
{
	populated = true;
}