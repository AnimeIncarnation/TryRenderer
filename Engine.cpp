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

#include "stdafx.h"
#include "Engine.h"
#include "Utilities/VertexStructDescriber.h"
#include "Resources/UploadBuffer.h"
#include "Resources/Mesh.h"
#include "Resources/BufferView.h"
#include "iostream"

D3D12HelloConstBuffers::D3D12HelloConstBuffers(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name),
    m_frameIndex(0),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_rtvDescriptorSize(0),
    m_constantBufferData{}
{
}

void D3D12HelloConstBuffers::OnInit()
{
    LoadPipeline();
    LoadAssets();
}

// ����������У���������RTV��CBV���������ѣ�����RTV���д�����������������
void D3D12HelloConstBuffers::LoadPipeline()
{
    //�㡢�����豸
    {
        dxDevice = std::make_unique<DXDevice>();

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;     //

        ThrowIfFailed(dxDevice->Device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    }

    //һ������������
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = FrameCount;
        swapChainDesc.Width = m_width;
        swapChainDesc.Height = m_height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(dxDevice->Factory()->CreateSwapChainForHwnd(
            m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
            Win32Application::GetHwnd(),
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain
        ));
        // This sample does not support fullscreen transitions.
        ThrowIfFailed(dxDevice->Factory()->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));
        ThrowIfFailed(swapChain.As(&m_swapChain));
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    }
  
    //���������������ѣ�����RTV�ѣ�CBV�ѣ����������RTV
    {
        //1. ������������
        {
            // Describe and create a render target view (RTV) descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = FrameCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(dxDevice->Device()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

            m_rtvDescriptorSize = dxDevice->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            // Describe and create a constant buffer view (CBV) descriptor heap.
            // Flags indicate that this descriptor heap can be bound to the pipeline 
            // and that descriptors contained in it can be referenced by a root table.
            D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
            cbvHeapDesc.NumDescriptors = FrameCount * 2;
            cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            ThrowIfFailed(dxDevice->Device()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
        }
        // 2. �ڶ��д���swapChain��RTV
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV for each frame.
            for (UINT n = 0; n < FrameCount; n++)
            {
                ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
                dxDevice->Device()->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
                //handleָ��RTV���е�RTV��������ƫ����
                rtvHandle.Offset(1, m_rtvDescriptorSize);
            }
        }
    }
   
    //����������Ȼ�����
 /*   {
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC texDesc;
        memset(&texDesc, 0, sizeof(D3D12_RESOURCE_DESC));
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Alignment = 0;
        texDesc.Width = m_scissorRect.right;
        texDesc.Height = m_scissorRect.bottom;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_D32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE defaultClearValue;
        defaultClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        defaultClearValue.DepthStencil.Depth = 1;
        ThrowIfFailed(dxDevice->Device()->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_DEPTH_READ,
            &defaultClearValue,
            IID_PPV_ARGS(&m_depthTarget)));
        dxDevice->Device()->CreateDepthStencilView(m_depthTarget.Get(), nullptr, dsvHandle);
    }*/

    //�ġ�����֡��Դ������������Ҳ�ڴ˴�����ϣ�
    {
        for (auto&& i : frameResources)
        {
            i = std::make_unique<FrameResource>(dxDevice.get(), constantBufferSize);
        }
        //currentCommandListHandle = frameResources[m_frameIndex]->Command();

    }

}

// ������ǩ��������ݣ�VS��PS��PSO�������б�ͬ����fence
void D3D12HelloConstBuffers::LoadAssets()
{
    auto& cmdList = frameResources[m_frameIndex]->cmdList;
    auto& cmdAlloc = frameResources[m_frameIndex]->cmdAllocator;
    //һ������Mesh�������洢�����Resource����
    //���ֶ�Ӳ���붥�����ݡ�
    //�ڴ���UploadBuffer�������ݴ�CPU�˿�����GPU�ˡ�
    //�۴���Mesh�������ݴ�UploadBuffer������Mesh�е�DefaultBuffer��
    {
        //CPU��VertexBuffer
        std::vector<byte> triangleVertices(Vertex::Instance().structSize * 6);
        byte* vertexBytePtr = triangleVertices.data();
        //ע���һ������
        Vertex::Instance().position.Get(vertexBytePtr) = { 0.0f, 0.25f , 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 1.0f, 0.0f, 0.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;
        //ע��ڶ�������
        Vertex::Instance().position.Get(vertexBytePtr) = { 0.25f, -0.25f, 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 0.0f, 1.0f, 0.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;
        //ע�����������
        Vertex::Instance().position.Get(vertexBytePtr) = { -0.25f, -0.25f, 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 0.0f, 0.0f, 1.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;
        //ע����ĸ�����
        Vertex::Instance().position.Get(vertexBytePtr) = { 0.0f, -0.25f , 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 1.0f, 0.0f, 0.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;
        //ע����������
        Vertex::Instance().position.Get(vertexBytePtr) = { 0.25f, -0.75f, 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 0.0f, 1.0f, 0.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;
        //ע�����������
        Vertex::Instance().position.Get(vertexBytePtr) = { -0.25f, -0.75f, 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 0.0f, 0.0f, 1.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;

        //����UploadBuffer�������Ͷ������ݵ�UploadBuffer
        uploadBufferVertex = std::make_unique<UploadBuffer>(dxDevice.get(), triangleVertices.size());
        uploadBufferVertex->CopyData(0, triangleVertices);  //offsetΪ0�����ݴ�triangleVertices�п���

        //����Mesh����UploadBuffer���ݿ�����DefaultBuffer
        std::vector<const rtti::ElementStruct *> structs;
        structs.emplace_back(&Vertex::Instance());
        triangleMesh = std::make_unique<Mesh>(dxDevice.get(), structs, 6);
        ID3D12Resource* dftbfr = triangleMesh->VertexBuffers()[0].GetResource();
        ThrowIfFailed(cmdList->Reset(cmdAlloc.Get(), m_pipelineState.Get()));
        cmdList->ResourceBarrier(1,
            get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
                dftbfr,
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST)));
        cmdList->CopyBufferRegion(dftbfr, 0, uploadBufferVertex->GetResource(), 0, triangleVertices.size());
        cmdList->ResourceBarrier(1,
            get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
                dftbfr,
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_COMMON)));
    }

    //������ÿ��֡��Դ�����������������������������������ݳ�ʼ����Upload + Default�ṹ����
    {
        m_cbvDescriptorSize = dxDevice->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        //��ʼ������������������Ҫ�ȴ���View
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
        for (auto&& i : frameResources)
        {        
             D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
             cbvDesc.BufferLocation = i->perObjConstantUpload->GetGPUAddress();
             cbvDesc.SizeInBytes = constantBufferSize;
             dxDevice->Device()->CreateConstantBufferView(&cbvDesc, cbvHandle);
             cbvHandle.Offset(1, m_cbvDescriptorSize);

             cbvDesc.BufferLocation = i->perObjConstantDefault->GetGPUAddress();
             cbvDesc.SizeInBytes = constantBufferSize;
             dxDevice->Device()->CreateConstantBufferView(&cbvDesc, cbvHandle);
             cbvHandle.Offset(1, m_cbvDescriptorSize);
        }


        //�����ڴ��еĳ������ݵ�Upload�������������ڴ��е�ӳ����
        for (int i = 0;i < FrameCount;i++)
        {
            //��Ϊ��������Unmap�����Բ�������UploadBuffer�ĳ�Ա�������ֶ����п���
            // Map and initialize the constant buffer. We don't unmap this until the
            // app closes. Keeping things mapped for the lifetime of the resource is okay.
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            ThrowIfFailed(frameResources[i]->perObjConstantUpload->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedCbvData[i])));
            memcpy(m_mappedCbvData[i], &m_constantBufferData, sizeof(m_constantBufferData));
        }

        //��UploadBuffer���ݿ�����DefaultBuffer
        cmdList->ResourceBarrier(1,
            get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
                frameResources[m_frameIndex]->perObjConstantDefault->GetResource(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST)));
        cmdList->CopyBufferRegion(frameResources[m_frameIndex]->perObjConstantDefault->GetResource(), 0, frameResources[0]->perObjConstantUpload->GetResource(), 0, constantBufferSize);
        cmdList->ResourceBarrier(1,
            get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
                frameResources[m_frameIndex]->perObjConstantDefault->GetResource(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_COMMON)));

        //Execute�����б����ﲻ��ͬ������LoadAssets����������ʱ��һ��ͬ��
        ThrowIfFailed(cmdList->Close());
        ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
        m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);
    }

    //����������ǩ��������һ�������������д���һ��CBV����
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(dxDevice->Device()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        //����һ�����������range�����������п���ѡ���������������������͸�������ÿ����һ��������������Ҫ�ṩһ��range��rangeָ�����ͣ��������Ĵ���
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];   
            //baseShaderRegister��numָ����GPU Buffer��shader register�Ķ�Ӧ��ϵ��һ��buffer��Ӧһ���Ĵ�����
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        //�������������ʼ��Ϊ����������ô����Ҫһ������descriptor_rangeָ�����������еķ�Χ��ָ���ĸ�shader�ɷ��ʸø�����
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

        // Allow input layout and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        //D3D12ָ�������Ƚ���ǩ�������������ֽ������л�����ת��Ϊ��blob�ӿڱ�ʾ�����л�����֮��ſɴ���CreateRootSignature��
        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(dxDevice->Device()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    //�ġ����롢����Shader����������״̬
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        //ע��ʹ��GetAssetFullPathʱThrow����ֱ��д��shaders.hlsl����������
        ThrowIfFailed(D3DCompileFromFile(std::wstring(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(std::wstring(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};                            //����״̬���������
        psoDesc.InputLayout = { triangleMesh->Layout().data(),
                                triangleMesh->Layout().size() };                    //����ṹ�岼�֡����Զ����Mesh���ȡ
        psoDesc.pRootSignature = m_rootSignature.Get();                             //��ǩ����shader������
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());                   //VS��DX���Զ����VS������͡�����ṹ�岼�֡��Ƿ�ƥ�䣬����һ��Ҫ>=ǰ�ߣ���ᣩ
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());                    //PS
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);           //��Ⱦ״̬��Ӳ��֧�ֵ�MSAA�������޳����߿�ģʽ����Ӱbias�����ع�դ������
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                     //���״̬����Ϸ�ʽ���Ƿ���AlphaToCoverage��MRTʱ�Ķ���״̬����
        psoDesc.DepthStencilState.DepthEnable = FALSE;                              //��Ȳ��ԣ�
        psoDesc.DepthStencilState.StencilEnable = FALSE;                            //ģ����ԣ�
        psoDesc.SampleMask = UINT_MAX;                                              //���ز��������������������롣32λbit
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;     //ͼԪ���ˡ�����ƶ���HS��DS������ΪPatch
        psoDesc.NumRenderTargets = 1;                                               //RenderTarget����������RTVFormats���鳤��
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                         //RenderTarget��ģʽ��ÿ��RenderTarget�ĸ�ʽ
        psoDesc.SampleDesc.Count = 1;                                               //ָ�����ز����Ĳ�����������������������������ȾĿ��Ķ�Ӧ������ͬ

        ThrowIfFailed(dxDevice->Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    //�塢����ͬ����Χ�����ȴ�ǰLoadAssets�׶����
    {
        ThrowIfFailed(dxDevice->Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;
        m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
        if (m_fence->GetCompletedValue() < m_fenceValue)
        {
            LPCWSTR falseValue = 0;
            HANDLE eventHandle = CreateEventEx(nullptr, falseValue, false, EVENT_ALL_ACCESS);
            ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, eventHandle));
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
    }
}

// Update frame-based values.
void D3D12HelloConstBuffers::OnUpdate(FrameResource& frameRes, UINT64 frameIndex)
{
    const float translationSpeed = 0.005f;
    const float offsetBounds = 1.25f;

    m_constantBufferData.offset.x += translationSpeed;
    if (m_constantBufferData.offset.x > offsetBounds)
    {
        m_constantBufferData.offset.x = -offsetBounds;
    }
    memcpy(m_mappedCbvData[frameIndex], &m_constantBufferData, sizeof(m_constantBufferData));


    //��ȡ��ǰ֡����Ϣ
    ID3D12CommandAllocator* cmdAlloc = frameRes.cmdAllocator.Get();
    ID3D12GraphicsCommandList* cmdList = frameRes.cmdList.Get();
    UploadBuffer* currUpload = frameRes.perObjConstantUpload.get();

    //PS������ʹ��DefaultBuffer������������ʱ�رճ����Ī������������Ⱦ����Ҳ�����ˣ����Ի���ʹ��UploadBuffer
    ////ÿ�θ��³�������������ʱ����UploadBuffer���ݿ�����DefaultBuffer
    //ThrowIfFailed(cmdList->Reset(cmdAlloc, m_pipelineState.Get()));
    //const UINT constantBufferSize = sizeof(SceneConstantBuffer);
    //cmdList->ResourceBarrier(1,
    //    get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
    //        frameResources[frameIndex]->perObjConstantDefault->GetResource(),
    //        D3D12_RESOURCE_STATE_COMMON,
    //        D3D12_RESOURCE_STATE_COPY_DEST)));
    //cmdList->CopyBufferRegion(frameResources[frameIndex]->perObjConstantDefault->GetResource(), 0, currUpload->GetResource(), 0, constantBufferSize);
    //cmdList->ResourceBarrier(1,
    //    get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
    //        frameResources[frameIndex]->perObjConstantDefault->GetResource(),
    //        D3D12_RESOURCE_STATE_COPY_DEST,
    //        D3D12_RESOURCE_STATE_COMMON)));

    ////Execute�����б����ﲻ��ͬ�����ȵ�Render����������һ��ͬ��
    //ThrowIfFailed(cmdList->Close());
    //ID3D12CommandList* ppCommandLists[] = { cmdList };
    //m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);
    
}

// ��Ⱦ������ÿ֡������Update
void D3D12HelloConstBuffers::OnRender()
{
    UINT currentFrameIndex = m_frameIndex;
    UINT nextFrameIndex = (m_frameIndex + 1) % FrameCount;
    UINT lastFrameIndex = (m_frameIndex + 2) % FrameCount;

    // Execute��ǰ֡��Present��������ź���
    frameResources[currentFrameIndex]->Execute(m_commandQueue.Get(), m_fenceValue);
    ThrowIfFailed(m_swapChain->Present(0, 0));
    m_frameIndex = (m_frameIndex + 1) % FrameCount;
    frameResources[currentFrameIndex]->Signal(m_commandQueue.Get(), m_fence.Get());

    // ������һ֡�ĳ�����������Populate��һ֡������
    OnUpdate(*frameResources[nextFrameIndex], nextFrameIndex);
    PopulateCommandList(*frameResources[nextFrameIndex], nextFrameIndex);

    // Sync(�ȴ�)��һ֡��ִ�С���Ϊ����Ҫ�������ĳ����������ˣ�����֮ǰ����������ִ�к�
    frameResources[lastFrameIndex]->Sync(m_fence.Get());
}

void D3D12HelloConstBuffers::OnDestroy()
{
    for (UINT i = (m_frameIndex + FrameCount - 1) % FrameCount; i != (m_frameIndex + 1) % FrameCount;i++)
    {
        frameResources[i]->Sync(m_fence.Get());
    }
}

// Fill the command list with all the render commands and dependent state.
void D3D12HelloConstBuffers::PopulateCommandList(FrameResource& frameRes, UINT64 frameIndex)
{
    //IA��Input Assembler��OM��Output Merger��RS��Rasterize Stage

    //��ȡ��ǰ֡����Ϣ
    frameRes.Populate();
    ID3D12GraphicsCommandList* cmdList = frameRes.cmdList.Get();
    ID3D12CommandAllocator* cmdAlloc = frameRes.cmdAllocator.Get();
    UploadBuffer* uploadBuffer = frameRes.perObjConstantUpload.get();
    ThrowIfFailed(cmdList->Reset(cmdAlloc, m_pipelineState.Get()));

    //֮ǰ�������˸��������ɸ�������֯�ɸ�ǩ��������Ҫ���ø�ǩ������������������Ҫ�����������ѣ���ΪҪ���������Ѻ͸�������ϵ����
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
    cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    //�󶨺ø�ǩ�����������Ѻ�Ϊÿ�����������и�ֵ��ÿ�����βζ�Ҫ��һ����ʵ������ֵ��
    //��tableʵ�ʸ�ֵ��table�ĸ��������, BaseDescriptor������Ϊ�ڸ�ǩ�����Ѿ������˷�Χ������ֻ��Ҫһ��base�Ϳ�����
    //�������õ���DefaultBuffeForConstant��Descriptorλ�ã�����offset��FrameCount
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), frameIndex * 2, m_cbvDescriptorSize);
    cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle); 

    cmdList->RSSetViewports(1, &m_viewport);
    cmdList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, m_rtvDescriptorSize);
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);


    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;
    triangleMesh->GetVertexBufferView(vertexBufferView);
    cmdList->IASetVertexBuffers(0, vertexBufferView.size(), vertexBufferView.data());
    cmdList->DrawInstanced(//���������������������Ҫʹ��DrawIndexedInstanced~���˿�����~
        6, //VertexCountPerInstance��ÿ��InstanceҪ���ƵĶ���������VertexCountPerInstanceָ���˱����Ƶ��ܶ������������Ƴ�ʲô������ͼԪ���˾���
        1, //InstanceCount��ʵ�����������������Ҫ��ʵ����������Ϊ1��Ȼ���VertexCountPerInstance��ΪVertexBuffer�ĳ��ȣ�StartVert��Ϊ�㼴�ɡ������Ҫ������Կ����������ⶨ�ư�����
        0, //StartVertexLocation�����㻺�����ڵ�һ�������ƵĶ��������������ʼ��
        0);//StartInstanceLocation������

    // Indicate that the back buffer will now be used to present.
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));

    ThrowIfFailed(cmdList->Close());
}