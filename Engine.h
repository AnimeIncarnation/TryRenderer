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
#include "Resources/Mesh.h"
#include "Resources/UploadBuffer.h"
#include "RenderBase/FrameResource.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class D3D12HelloConstBuffers : public DXSample
{
public:
    D3D12HelloConstBuffers(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate(FrameResource& frameRes, UINT64 frameIndex);
    virtual void OnRender();
    virtual void OnDestroy();

private:
    static const UINT FrameCount = 3;
    const UINT constantBufferSize = sizeof(SceneConstantBuffer);    // CB size is required to be 256-byte aligned.
    struct Vertex: rtti::ElementStruct
    {
        rtti::ElementType<XMFLOAT3> position = "POSITION";
        rtti::ElementType<XMFLOAT4> color = "COLOR";
        static Vertex& Instance()
        {
            static Vertex instance;
            return instance;
        }
    private:
        Vertex() {};
    };

    struct SceneConstantBuffer
    {
        XMFLOAT4 offset;
        float padding[60]; // Padding so the constant buffer is 256-byte aligned.
    };
    static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

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
    ComPtr<ID3D12PipelineState> m_pipelineState;
    UINT m_rtvDescriptorSize;
    UINT m_cbvDescriptorSize;

    // App resources.
    std::unique_ptr<UploadBuffer>  m_uploadBufferForConstant;
    
    SceneConstantBuffer m_constantBufferData;
    UINT8* m_mappedCbvData[FrameCount];
    std::unique_ptr<Mesh> triangleMesh;
    std::unique_ptr<UploadBuffer> uploadBufferVertex;

    // Synchronization objects.
    // CommandListHandle currentCommandListHandle;
    UINT m_frameIndex;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList(FrameResource& frameRes, UINT64 frameIndex);
};
