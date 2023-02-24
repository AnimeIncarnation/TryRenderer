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

Engine::Engine(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name),
    m_frameIndex(0),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_rtvDescriptorSize(0),
    m_constantBufferData{}
{
}

void Engine::OnInit()
{
    LoadPipeline();
    LoadAssets();
}

// ����������У���������RTV��CBV���������ѣ�����RTV���д�����������������
void Engine::LoadPipeline()
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
  
    //���������������ѣ�RTV�ѣ�CBV�ѣ�DSV�ѣ����������RTV
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
            m_cbvDescriptorSize = dxDevice->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
            dsvHeapDesc.NumDescriptors = 1;
            dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(dxDevice->Device()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
            m_dsvDescriptorSize = dxDevice->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        }
        // 2. �ڶ��д���RTV
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
   
    //����������Ȼ�������DSV
    {
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
    }

    //�ġ�����֡��Դ������������Ҳ�ڴ˴�����ϣ�
    {
        for (auto&& i : frameResources)
        {
            i = std::make_unique<FrameResource>(dxDevice.get(), constantBufferSize);
        }
        //currentCommandListHandle = frameResources[m_frameIndex]->Command();

    }

}
static XMFLOAT3 vertices[] = {
    XMFLOAT3(0.5, -0.5, 0.5),
    XMFLOAT3(-0.5, -0.5, 0.5),
    XMFLOAT3(0.5, 0.5, 0.5),
    XMFLOAT3(-0.5, 0.5, 0.5),
    XMFLOAT3(0.5, 0.5, -0.5),
    XMFLOAT3(-0.5, 0.5, -0.5),
    XMFLOAT3(0.5, -0.5, -0.5),
    XMFLOAT3(-0.5, -0.5, -0.5),
    XMFLOAT3(0.5, 0.5, 0.5),
    XMFLOAT3(-0.5, 0.5, 0.5),
    XMFLOAT3(0.5, 0.5, -0.5),
    XMFLOAT3(-0.5, 0.5, -0.5),
    XMFLOAT3(0.5, -0.5, -0.5),
    XMFLOAT3(0.5, -0.5, 0.5),
    XMFLOAT3(-0.5, -0.5, 0.5),
    XMFLOAT3(-0.5, -0.5, -0.5),
    XMFLOAT3(-0.5, -0.5, 0.5),
    XMFLOAT3(-0.5, 0.5, 0.5),
    XMFLOAT3(-0.5, 0.5, -0.5),
    XMFLOAT3(-0.5, -0.5, -0.5),
    XMFLOAT3(0.5, -0.5, -0.5),
    XMFLOAT3(0.5, 0.5, -0.5),
    XMFLOAT3(0.5, 0.5, 0.5),
    XMFLOAT3(0.5, -0.5, 0.5) 
}; 
static UINT indices[] = { 0, 2, 3, 0, 3, 1, 8, 4, 5, 8, 5, 9, 10, 6, 7, 10, 7, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23 };
//static XMFLOAT3 vertices[] = {
//    XMFLOAT3(0.5, -0.5, 0.5),      //red
//    XMFLOAT3(-0.5, 0.5, 0.5),      //blue
//    XMFLOAT3(0.5, 0.5, 0.5),       //white
//    XMFLOAT3(0.5, -0.5, 0.5),    
//    XMFLOAT3(-0.5, 0.5, 0.5),    
//    XMFLOAT3(-0.5, -0.5, 0.5),   
//    XMFLOAT3(0.5, 0.5, 0.5),     
//    XMFLOAT3(0.5, 0.5, -0.5),    
//    XMFLOAT3(-0.5, 0.5, -0.5),   
//    XMFLOAT3(0.5, 0.5, 0.5),     
//    XMFLOAT3(-0.5, 0.5, -0.5),   
//    XMFLOAT3(-0.5, 0.5, 0.5),    
//    XMFLOAT3(0.5, 0.5, -0.5),    
//    XMFLOAT3(0.5, -0.5, -0.5),   
//    XMFLOAT3(-0.5, -0.5, -0.5),  
//    XMFLOAT3(0.5, 0.5, -0.5),    
//    XMFLOAT3(-0.5, -0.5, -0.5),  
//    XMFLOAT3(-0.5, 0.5, -0.5),   
//    XMFLOAT3(0.5, -0.5, -0.5),   
//    XMFLOAT3(0.5, -0.5, 0.5),    
//    XMFLOAT3(-0.5, -0.5, 0.5),   
//    XMFLOAT3(0.5, -0.5, -0.5),   
//    XMFLOAT3(-0.5, -0.5, 0.5),   
//    XMFLOAT3(-0.5, -0.5, -0.5),  
//    XMFLOAT3(-0.5, -0.5, 0.5),   
//    XMFLOAT3(-0.5, 0.5, 0.5),    
//    XMFLOAT3(-0.5, 0.5, -0.5),   
//    XMFLOAT3(-0.5, -0.5, 0.5),   
//    XMFLOAT3(-0.5, 0.5, -0.5),   
//    XMFLOAT3(-0.5, -0.5, -0.5),  
//    XMFLOAT3(0.5, -0.5, -0.5),   
//    XMFLOAT3(0.5, 0.5, -0.5),    
//    XMFLOAT3(0.5, 0.5, 0.5),     
//    XMFLOAT3(0.5, -0.5, -0.5),   
//    XMFLOAT3(0.5, 0.5, 0.5),     
//    XMFLOAT3(0.5, -0.5, 0.5) 
//};  
//static UINT indices[] = { 0,1, 2, 3, 4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35 };


// ������ǩ��������ݣ�VS��PS��PSO�������б�ͬ����fence
void Engine::LoadAssets()
{
    auto& cmdList = frameResources[m_frameIndex]->cmdList;
    auto& cmdAlloc = frameResources[m_frameIndex]->cmdAllocator;

    //�㡢�������
    mainCamera = std::make_unique<Camera>();
    Math::Vector3 up = Math::Vector3(-0.3181089, 0.8988663, 0.301407);
    Math::Vector3 forward = Math::Vector3(-0.6525182, -0.438223, 0.6182076);
    Math::Vector3 pos = Math::Vector3(2.232773, 1.501817, -1.883978);
    mainCamera->LookAt(pos, forward, up);
    mainCamera->aspect = static_cast<float>(m_scissorRect.right) / static_cast<float>(m_scissorRect.bottom);
    mainCamera->UpdateViewMatrix();
    mainCamera->UpdateProjectionMatrix();

    //һ������Mesh�������洢�����Resource����
    //�ٻ�ȡ�������������
    //�ڴ���UploadBuffer�������ݴ�CPU�˿�����GPU�ˡ�
    //�۴���Mesh�������ݴ�UploadBuffer������Mesh�е�DefaultBuffer��
    {
        
        //CPU��VertexBuffer
        std::vector<byte> cubeVertices(Vertex::Instance().structSize * array_count(vertices));
        byte* vertexBytePtr = cubeVertices.data();
        for (int i = 0;i < array_count(vertices);i++)
        {
            XMFLOAT3 vert = vertices[i];
            Vertex::Instance().position.Get(vertexBytePtr) = vert;
            XMFLOAT4 color(vert.x + 0.5f, vert.y + 0.5f, vert.z + 0.5f, 1);
            Vertex::Instance().color.Get(vertexBytePtr) = color;
            vertexBytePtr += Vertex::Instance().structSize;
        }

        //����UploadBuffer�������Ͷ������ݵ�UploadBuffer
        uploadBufferVertex = std::make_unique<UploadBuffer>(dxDevice.get(), cubeVertices.size());
        uploadBufferIndex = std::make_unique<UploadBuffer>(dxDevice.get(), array_count(indices) * sizeof(UINT));
        uploadBufferVertex->CopyData(0, cubeVertices);  //offsetΪ0�����ݴ�cubeVertices�п���
        uploadBufferIndex->CopyData(0, { reinterpret_cast<byte const*>(indices),array_count(indices) * sizeof(UINT) });  //offsetΪ0�����ݴ�cubeVertices�п���


        //����Mesh����UploadBuffer���ݿ�����DefaultBuffer
        std::vector<const rtti::ElementStruct *> structs;
        structs.emplace_back(&Vertex::Instance());
        cubeMesh = std::make_unique<Mesh>(dxDevice.get(), structs, array_count(vertices), array_count(indices));
        ID3D12Resource* dftbfr = cubeMesh->VertexBuffers()[0].GetResource();
        ThrowIfFailed(cmdList->Reset(cmdAlloc.Get(), m_pipelineState.Get()));

        cmdList->CopyBufferRegion(dftbfr, 0, uploadBufferVertex->GetResource(), 0, uploadBufferVertex->GetSize());
        cmdList->CopyBufferRegion(cubeMesh->IndexBuffer().GetResource(), 0, uploadBufferIndex->GetResource(), 0, uploadBufferIndex->GetSize());
    }

    //������ÿ��֡��Դ�����������������������������������ݳ�ʼ����Upload + Default�ṹ����
    {
        //��ʼ������������������Ҫ�ȴ���View
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
        for (auto&& i : frameResources)
        {        
             D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
             cbvDesc.BufferLocation = i->constantUpload->GetGPUAddress();
             cbvDesc.SizeInBytes = constantBufferSize;
             dxDevice->Device()->CreateConstantBufferView(&cbvDesc, cbvHandle);
             cbvHandle.Offset(1, m_cbvDescriptorSize);

             cbvDesc.BufferLocation = i->constantDefault->GetGPUAddress();
             cbvDesc.SizeInBytes = constantBufferSize;
             dxDevice->Device()->CreateConstantBufferView(&cbvDesc, cbvHandle);
             cbvHandle.Offset(1, m_cbvDescriptorSize);
        }


        //�����ڴ��еĳ������ݵ�Upload�������������ڴ��е�ӳ���У�Map + memcpy
        for (int i = 0;i < FrameCount;i++)
        {
            // ��Ϊ��������Unmap�����Բ�������UploadBuffer�ĳ�Ա�������ֶ����п���
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            ThrowIfFailed(frameResources[i]->constantUpload->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedCbvData[i])));
            memcpy(m_mappedCbvData[i], &m_constantBufferData, sizeof(m_constantBufferData));
        }

        //��UploadBuffer���ݿ�����DefaultBuffer
        cmdList->ResourceBarrier(1,
            get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
                frameResources[m_frameIndex]->constantDefault->GetResource(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST)));
        cmdList->CopyBufferRegion(frameResources[m_frameIndex]->constantDefault->GetResource(), 0, frameResources[0]->constantUpload->GetResource(), 0, constantBufferSize);
        cmdList->ResourceBarrier(1,
            get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
                frameResources[m_frameIndex]->constantDefault->GetResource(),
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
        ThrowIfFailed(D3DCompileFromFile(std::wstring(L"Shaders/shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(std::wstring(L"Shaders/shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, nullptr));
        
        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};                            //����״̬���������
        psoDesc.InputLayout = { cubeMesh->Layout().data(),
                                cubeMesh->Layout().size() };                        //����ṹ�岼�֡����Զ����Mesh���ȡ
        psoDesc.pRootSignature = m_rootSignature.Get();                             //��ǩ����shader������
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());                   //VS��DX���Զ����VS������͡�����ṹ�岼�֡��Ƿ�ƥ�䣬����һ��Ҫ>=ǰ�ߣ���ᣩ
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());                    //PS
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);           //��Ⱦ״̬��Ӳ��֧�ֵ�MSAA�������޳����߿�ģʽ����Ӱbias�����ع�դ������
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                     //���״̬����Ϸ�ʽ���Ƿ���AlphaToCoverage��MRTʱ�Ķ���״̬����
        psoDesc.DepthStencilState.DepthEnable = TRUE;                               //���д���
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;           //С����д��
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.StencilEnable = FALSE;                            //ģ����ԣ�
        psoDesc.SampleMask = UINT_MAX;                                              //���ز��������������������롣32λbit
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;     //ͼԪ���ˡ�����ƶ���HS��DS������ΪPatch
        psoDesc.NumRenderTargets = 1;                                               //RenderTarget����������RTVFormats���鳤��
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                         //RenderTarget��ģʽ��ÿ��RenderTarget�ĸ�ʽ
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
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
void Engine::OnUpdate(FrameResource& frameRes, UINT64 frameIndex)
{
    mainCamera->UpdateViewMatrix();
    mainCamera->UpdateProjectionMatrix();
    m_constantBufferData.viewMatrix = mainCamera->GetViewMatrix();
    m_constantBufferData.projMatrix = mainCamera->GetProjectionMatrix();
    m_constantBufferData.vpMatrix = m_constantBufferData.projMatrix * m_constantBufferData.viewMatrix;
    memcpy(m_mappedCbvData[frameIndex], &m_constantBufferData, sizeof(m_constantBufferData));


    //��ȡ��ǰ֡����Ϣ
    ID3D12CommandAllocator* cmdAlloc = frameRes.cmdAllocator.Get();
    ID3D12GraphicsCommandList* cmdList = frameRes.cmdList.Get();
    UploadBuffer* currUpload = frameRes.constantUpload.get();

    //PS������ʹ��DefaultBuffer������������ʱ�رճ����Ī������������Ⱦ����Ҳ�����ˣ����Ի���ʹ��UploadBuffer
    //ÿ�θ��³�������������ʱ����UploadBuffer���ݿ�����DefaultBuffer
    ThrowIfFailed(cmdAlloc->Reset());
    ThrowIfFailed(cmdList->Reset(cmdAlloc, m_pipelineState.Get()));

    const UINT constantBufferSize = sizeof(PerCameraConstant);
    
    cmdList->ResourceBarrier(1,
        get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
            frameResources[frameIndex]->constantDefault->GetResource(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST)));
    cmdList->CopyBufferRegion(frameResources[frameIndex]->constantDefault->GetResource(), 0, currUpload->GetResource(), 0, constantBufferSize);
    cmdList->ResourceBarrier(1,
        get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
            frameResources[frameIndex]->constantDefault->GetResource(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COMMON)));

    //���ﲻExecute���Ⱥ�populateһ��ִ�С���Ȼ���������Ҫͬ������

    //ThrowIfFailed(cmdList->Close());
    //ID3D12CommandList* ppCommandLists[] = { cmdList };
    //m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);
}

// ��Ⱦ������ÿ֡������Update
void Engine::OnRender()
{
    UINT currentFrameIndex = m_frameIndex;
    UINT nextFrameIndex = (m_frameIndex + 1) % FrameCount;

    // Execute��ǰ֡��Present��������ź���
    frameResources[currentFrameIndex]->Execute(m_commandQueue.Get(), m_fenceValue);
    ThrowIfFailed(m_swapChain->Present(0, 0));
    m_frameIndex = (m_frameIndex + 1) % FrameCount;
    frameResources[currentFrameIndex]->Signal(m_commandQueue.Get(), m_fence.Get());

    // Sync(�ȴ�)��һ֡��ִ�С���Ϊ����Ҫ�������ĳ����������ˣ�����֮ǰ����������ִ�к�
    frameResources[nextFrameIndex]->Sync(m_fence.Get());

    // ������һ֡�ĳ�����������Populate��һ֡������
    OnUpdate(*frameResources[nextFrameIndex], nextFrameIndex);
    PopulateCommandList(*frameResources[nextFrameIndex], nextFrameIndex);

}

void Engine::OnDestroy()
{
    for (UINT i = (m_frameIndex + FrameCount - 1) % FrameCount; i != (m_frameIndex + 1) % FrameCount;i = (i + 1) % FrameCount)
    {
        frameResources[i]->Sync(m_fence.Get());
    }
}

// Fill the command list with all the render commands and dependent state.
void Engine::PopulateCommandList(FrameResource& frameRes, UINT64 frameIndex)
{
    //IA��Input Assembler��OM��Output Merger��RS��Rasterize Stage

    //��ȡ��ǰ֡����Ϣ
    frameRes.Populate();
    ID3D12GraphicsCommandList* cmdList = frameRes.cmdList.Get();
    ID3D12CommandAllocator* cmdAlloc = frameRes.cmdAllocator.Get();
    UploadBuffer* uploadBuffer = frameRes.constantUpload.get();
    //ThrowIfFailed(cmdList->Reset(cmdAlloc, m_pipelineState.Get()));

    //֮ǰ�������˸��������ɸ�������֯�ɸ�ǩ��������Ҫ���ø�ǩ������������������Ҫ�����������ѣ���ΪҪ���������Ѻ͸�������ϵ����
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
    cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    //�󶨺ø�ǩ�����������Ѻ�Ϊÿ�����������и�ֵ��ÿ�����βζ�Ҫ��һ����ʵ������ֵ��
    //��tableʵ�ʸ�ֵ��table�ĸ��������, BaseDescriptor������Ϊ�ڸ�ǩ�����Ѿ������˷�Χ������ֻ��Ҫһ��base�Ϳ�����
    //�������õ���DefaultBuffeForConstant��Descriptorλ�ã�����offset��FrameCount
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), frameIndex * 2 + 1, m_cbvDescriptorSize);
    cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle); 

    ////1. ִ��+����
    //ThrowIfFailed(cmdList->Close());
    //ID3D12CommandList* ppCommandLists[] = { cmdList };
    //m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);
    //m_commandQueue->Signal(m_fence.Get(), ++m_fenceValue);
    //if (m_fence->GetCompletedValue() < m_fenceValue)
    //{
    //    LPCWSTR falseValue = 0;
    //    HANDLE eventHandle = CreateEventEx(nullptr, falseValue, false, EVENT_ALL_ACCESS);
    //    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, eventHandle));
    //    WaitForSingleObject(eventHandle, INFINITE);
    //    CloseHandle(eventHandle);
    //}
    //ThrowIfFailed(cmdList->Reset(cmdAlloc, m_pipelineState.Get()));
    ////1. ִ��+����+ͬ��
    cmdList->RSSetViewports(1, &m_viewport);
    cmdList->RSSetScissorRects(1, &m_scissorRect);
    
    // Indicate that the back buffer will be used as a render target.
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_depthTarget.Get(), D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE)));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_dsvDescriptorSize);
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);


    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;
    cubeMesh->GetVertexBufferView(vertexBufferView);

    cmdList->IASetVertexBuffers(0, vertexBufferView.size(), vertexBufferView.data());
    D3D12_INDEX_BUFFER_VIEW indexBufferView = cubeMesh->GetIndexBufferView();
    cmdList->IASetIndexBuffer(&indexBufferView);
    cmdList->DrawIndexedInstanced(array_count(indices), 1, 0, 0, 0);
    //cmdList->DrawInstanced(//���������������������Ҫʹ��DrawIndexedInstanced~���˿�����~
    //    array_count(vertices), //VertexCountPerInstance��ÿ��InstanceҪ���ƵĶ���������VertexCountPerInstanceָ���˱����Ƶ��ܶ������������Ƴ�ʲô������ͼԪ���˾���
    //    1, //InstanceCount��ʵ�����������������Ҫ��ʵ����������Ϊ1��Ȼ���VertexCountPerInstance��ΪVertexBuffer�ĳ��ȣ�StartVert��Ϊ�㼴�ɡ������Ҫ������Կ����������ⶨ�ư�����
    //    0, //StartVertexLocation�����㻺�����ڵ�һ�������ƵĶ��������������ʼ��
    //    0);//StartInstanceLocation������

    // Indicate that the back buffer will now be used to present.
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_depthTarget.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ)));

    ThrowIfFailed(cmdList->Close());
}