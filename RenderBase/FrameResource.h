#pragma once
#ifndef _FRAME_RESOURCE_
#define _FRAME_RESOURCE_

#include <functional>
#include "../stdafx.h"
#include "../DXSampleHelper.h"
#include "../Resources/UploadBuffer.h"
#include "../Resources/DefaultBuffer.h"
#include "../Resources/Mesh.h"
#include "../DXMath/MathHelper.h"
#include "../Component/PSOManager.h"
#include "../Component/RasterShader.h"








//存储以帧为单位varying的量
class FrameResource
{
public:
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
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
    void Signal(ID3D12CommandQueue* queue, ID3D12Fence* fence); //在queue中注入更新信号量的任务
    void Sync(ID3D12Fence* fence);
    void SetRenderTarget(   //设置OM和RS
        CD3DX12_VIEWPORT const* viewport,
        CD3DX12_RECT const* scissorRect,
        CD3DX12_CPU_DESCRIPTOR_HANDLE const* rtvHandle,
        CD3DX12_CPU_DESCRIPTOR_HANDLE const* dsvHandle = nullptr);
    void ClearRenderTarget(const CD3DX12_CPU_DESCRIPTOR_HANDLE& rtv);   //Clear RT操作
    void ClearDepthStencilBuffer(const CD3DX12_CPU_DESCRIPTOR_HANDLE& dsv); //Clear DS操作
    void DrawMesh(  //IA，Shader参数设置 + Draw
        DXDevice* device,
        const RasterShader* shader, 
        PSOManager* psoManager, 
        Mesh* mesh,
        DXGI_FORMAT colorFormat,
        DXGI_FORMAT depthFormat);

};





#endif 