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
//#include "CommandListHandle.h"


//struct InstanceConstants
//{
//	int offset;
//};

struct CameraConstants
{
    /*DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProjTex = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ShadowTransform = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;

    DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };*/

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    //Light Lights[MaxLights];
};

struct PBRMatData
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.5f;

    // Used in texture mapping.
    DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

    UINT DiffuseMapIndex = 0;
    UINT NormalMapIndex = 0;
    UINT MaterialPad1;
    UINT MaterialPad2;
};




//存储以帧为单位varying的量
class FrameResource
{
public:
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
    std::unique_ptr<DefaultBuffer> constantDefault;
    std::unique_ptr<UploadBuffer> constantUpload;
	UINT64 fenceID = 0;
    bool populated = false;

    void Populate();
    FrameResource(DXDevice* device, UINT cbufferSize);
    ~FrameResource() = default;
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