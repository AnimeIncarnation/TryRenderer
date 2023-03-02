//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once
//#pragma comment(lib, "d3dcompiler.lib")
//#pragma comment(lib, "D3D12.lib")
//#pragma comment(lib, "dxgi.lib")


#include "DXSample.h"
#include "RenderBase/DXDevice.h"
#include "Utilities/VertexStructDescriber.h"
#include "Resources/Model.h"
#include "Resources/UploadBuffer.h"
#include "RenderBase/FrameResource.h"
#include "Component/Camera.h"
#include "Component/PSOManager.h"
#include "Component/Constants.h"
#include "Component/Light.h"
#include "RenderBase/ModelImporter.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class Engine : public DXSample
{
public:
    Engine(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate(FrameResource& frameRes, UINT64 frameIndex);
    virtual void OnRender();
    virtual void OnDestroy();

private:
    static const UINT FrameCount = 3;
    
    // Pipeline objects.
    std::unique_ptr<DXDevice> dxDevice;
    std::unique_ptr<FrameResource> frameResources[FrameCount];
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_depthTarget;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    UINT m_rtvDescriptorSize;
    UINT m_cbvDescriptorSize;
    UINT m_dsvDescriptorSize;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    std::unique_ptr<PSOManager> m_psoManager;
    std::unique_ptr<RasterShader> colorShader;

    // App resources.
    PerCameraConstant cameraConstantData;
    PerLightConstant lightConstantData;
    std::unique_ptr<ModelImporter> modelImporter;
    std::vector<Light> sceneLights;
    std::vector<Model> models;
    std::unique_ptr<Camera> mainCamera;

    // Synchronization objects.
    // CommandListHandle currentCommandListHandle;
    UINT m_frameIndex;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList(FrameResource& frameRes, UINT64 frameIndex);
    void OnMouseMove(WPARAM btnState, int x, int y);
    void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseUp(WPARAM btnState, int x, int y);
    void OnKeyDown(UINT8 key);
    void OnKeyUp(UINT8 key);
};
