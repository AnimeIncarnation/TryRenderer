#pragma once
#ifndef _FRAME_RESOURCE_
#define _FRAME_RESOURCE_

#include <functional>
#include "../stdafx.h"
#include "../DXSampleHelper.h"
#include "../Resources/UploadBuffer.h"
#include "../Resources/DefaultBuffer.h"
#include "../Resources/Model.h"
#include "../DXMath/MathHelper.h"
#include "../Component/PSOManager.h"
#include "../Component/RasterShader.h"








//�洢��֡Ϊ��λvarying����
class FrameResource
{
public:
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList6> cmdList; //CommandList6�汾����֧��MeshShader
    std::vector<std::unique_ptr<DefaultBuffer>> constantDefault;         //
    std::vector<std::unique_ptr<UploadBuffer>> constantUpload;           //
    std::vector<UINT> constantBufferSize;
    std::vector<UINT8*> mappedCbvData;                                   //
	UINT64 fenceID = 0;                                                  //
    bool populated = false;

    void CopyConstantFromUploadToDefault();
    void Populate();
    FrameResource(DXDevice* device);
    ~FrameResource() = default;
    void EmplaceConstantBuffer(DXDevice* device, UINT cbufferSize);
    void Execute(ID3D12CommandQueue* queue, uint64& fenceIndex);
    void Signal(ID3D12CommandQueue* queue, ID3D12Fence* fence); //��queue��ע������ź���������
    void Sync(ID3D12Fence* fence);
    void SetRenderTarget(   //����OM��RS
        CD3DX12_VIEWPORT const* viewport,
        CD3DX12_RECT const* scissorRect,
        CD3DX12_CPU_DESCRIPTOR_HANDLE const* rtvHandle,
        CD3DX12_CPU_DESCRIPTOR_HANDLE const* dsvHandle = nullptr);
    void ClearRenderTarget(const CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv);   //Clear RT����
    void ClearDepthStencilBuffer(const CD3DX12_CPU_DESCRIPTOR_HANDLE& dsv); //Clear DS����
    //����IA������PSO + Draw
    void DrawMesh(DXDevice* device, Model* model, ID3D12PipelineState* pipelineState);
    //void DrawMesh(DXDevice* device, D3D12_VERTEX_BUFFER_VIEW* vbView, D3D12_INDEX_BUFFER_VIEW* ibView, ID3D12PipelineState* pipelineState, UINT64 indexCount);
    void DrawMeshlet(DXDevice* device, Model* model, ID3D12PipelineState* pipelineState, RasterShader* shader);
};





#endif 