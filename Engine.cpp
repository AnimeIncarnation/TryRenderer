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

// 创建命令队列，交换链，RTV和CBV的描述符堆，并在RTV堆中创建交换链的描述符
void D3D12HelloConstBuffers::LoadPipeline()
{
    //零、创建设备
    {
        dxDevice = std::make_unique<DXDevice>();

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;     //

        ThrowIfFailed(dxDevice->Device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    }

    //一、创建交换链
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
  
    //二、创建描述符堆（包括RTV堆，CBV堆），创建多个RTV
    {
        //1. 创建描述符堆
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
        // 2. 在堆中创建swapChain的RTV
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV for each frame.
            for (UINT n = 0; n < FrameCount; n++)
            {
                ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
                dxDevice->Device()->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
                //handle指向RTV堆中的RTV，用来做偏移量
                rtvHandle.Offset(1, m_rtvDescriptorSize);
            }
        }
    }
   
    //三、创建深度缓冲区
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

    //四、创建帧资源（常量缓冲区也在此创建完毕）
    {
        for (auto&& i : frameResources)
        {
            i = std::make_unique<FrameResource>(dxDevice.get(), constantBufferSize);
        }
        //currentCommandListHandle = frameResources[m_frameIndex]->Command();

    }

}

// 创建根签名相关内容，VS，PS，PSO，命令列表，同步用fence
void D3D12HelloConstBuffers::LoadAssets()
{
    auto& cmdList = frameResources[m_frameIndex]->cmdList;
    auto& cmdAlloc = frameResources[m_frameIndex]->cmdAllocator;
    //一、建立Mesh（建立存储顶点的Resource）：
    //①手动硬编码顶点数据。
    //②创建UploadBuffer，把数据从CPU端拷贝到GPU端。
    //③创建Mesh，把数据从UploadBuffer拷贝到Mesh中的DefaultBuffer。
    {
        //CPU端VertexBuffer
        std::vector<byte> triangleVertices(Vertex::Instance().structSize * 6);
        byte* vertexBytePtr = triangleVertices.data();
        //注入第一个顶点
        Vertex::Instance().position.Get(vertexBytePtr) = { 0.0f, 0.25f , 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 1.0f, 0.0f, 0.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;
        //注入第二个顶点
        Vertex::Instance().position.Get(vertexBytePtr) = { 0.25f, -0.25f, 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 0.0f, 1.0f, 0.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;
        //注入第三个顶点
        Vertex::Instance().position.Get(vertexBytePtr) = { -0.25f, -0.25f, 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 0.0f, 0.0f, 1.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;
        //注入第四个顶点
        Vertex::Instance().position.Get(vertexBytePtr) = { 0.0f, -0.25f , 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 1.0f, 0.0f, 0.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;
        //注入第五个顶点
        Vertex::Instance().position.Get(vertexBytePtr) = { 0.25f, -0.75f, 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 0.0f, 1.0f, 0.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;
        //注入第六个顶点
        Vertex::Instance().position.Get(vertexBytePtr) = { -0.25f, -0.75f, 0.0f };
        Vertex::Instance().color.Get(vertexBytePtr) = { 0.0f, 0.0f, 1.0f, 1.0f };
        vertexBytePtr += Vertex::Instance().structSize;

        //创建UploadBuffer，并输送顶点数据到UploadBuffer
        uploadBufferVertex = std::make_unique<UploadBuffer>(dxDevice.get(), triangleVertices.size());
        uploadBufferVertex->CopyData(0, triangleVertices);  //offset为0，数据从triangleVertices中拷贝

        //创建Mesh，把UploadBuffer数据拷贝到DefaultBuffer
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

    //二、给每个帧资源创建创建常量缓冲区描述符，并进行数据初始化（Upload + Default结构）：
    {
        m_cbvDescriptorSize = dxDevice->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        //开始创建常量缓冲区——要先创建View
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


        //拷贝内存中的常量数据到Upload常量缓冲区在内存中的映射中
        for (int i = 0;i < FrameCount;i++)
        {
            //因为并不马上Unmap，所以并不适用UploadBuffer的成员函数，手动进行拷贝
            // Map and initialize the constant buffer. We don't unmap this until the
            // app closes. Keeping things mapped for the lifetime of the resource is okay.
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            ThrowIfFailed(frameResources[i]->perObjConstantUpload->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedCbvData[i])));
            memcpy(m_mappedCbvData[i], &m_constantBufferData, sizeof(m_constantBufferData));
        }

        //把UploadBuffer数据拷贝到DefaultBuffer
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

        //Execute命令列表，这里不用同步，等LoadAssets函数结束的时候一起同步
        ThrowIfFailed(cmdList->Close());
        ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
        m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);
    }

    //三、创建根签名（带有一个描述符表，表中带有一个CBV）：
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(dxDevice->Device()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        //建立一个根参数表和range表。根参数表中可以选择填入描述符表，描述符和根常量；每填入一个描述符表则需要提供一个range，range指定类型，数量，寄存器
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];   
            //baseShaderRegister和num指定了GPU Buffer和shader register的对应关系。一个buffer对应一个寄存器槽
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        //如果将根参数初始化为描述符表，那么就需要一个或多个descriptor_range指定描述符堆中的范围。指定哪个shader可访问该根参数
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

        //D3D12指定必须先将根签名的描述符布局进行序列化处理，转换为以blob接口表示的序列化数据之后才可传入CreateRootSignature中
        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(dxDevice->Device()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    //四、编译、加载Shader，创建管线状态
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        //注：使用GetAssetFullPath时Throw错误，直接写入shaders.hlsl后正常运行
        ThrowIfFailed(D3DCompileFromFile(std::wstring(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(std::wstring(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};                            //管线状态对象包括：
        psoDesc.InputLayout = { triangleMesh->Layout().data(),
                                triangleMesh->Layout().size() };                    //顶点结构体布局。用自定义的Mesh类获取
        psoDesc.pRootSignature = m_rootSignature.Get();                             //根签名（shader参数）
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());                   //VS。DX会自动检测VS的输入和“顶点结构体布局”是否匹配，后者一定要>=前者（意会）
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());                    //PS
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);           //渲染状态（硬件支持的MSAA？背面剔除？线框模式？阴影bias？保守光栅化？）
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                     //混合状态（混合方式？是否开启AlphaToCoverage？MRT时的多混合状态？）
        psoDesc.DepthStencilState.DepthEnable = FALSE;                              //深度测试？
        psoDesc.DepthStencilState.StencilEnable = FALSE;                            //模板测试？
        psoDesc.SampleMask = UINT_MAX;                                              //多重采样采样点允许与否的掩码。32位bit
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;     //图元拓扑。如果制定了HS和DS，必须为Patch
        psoDesc.NumRenderTargets = 1;                                               //RenderTarget的数量：即RTVFormats数组长度
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                         //RenderTarget的模式：每个RenderTarget的格式
        psoDesc.SampleDesc.Count = 1;                                               //指定多重采样的采样数量和质量级别。数量必须与渲染目标的对应数据相同

        ThrowIfFailed(dxDevice->Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    //五、创建同步用围栏，等待前LoadAssets阶段完成
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


    //获取当前帧的信息
    ID3D12CommandAllocator* cmdAlloc = frameRes.cmdAllocator.Get();
    ID3D12GraphicsCommandList* cmdList = frameRes.cmdList.Get();
    UploadBuffer* currUpload = frameRes.perObjConstantUpload.get();

    //PS：由于使用DefaultBuffer做常量缓冲区时关闭程序会莫名报错，而且渲染好像也变慢了，所以还是使用UploadBuffer
    ////每次更新常量缓冲区数据时，把UploadBuffer数据拷贝到DefaultBuffer
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

    ////Execute命令列表，这里不用同步，等到Render函数结束后一起同步
    //ThrowIfFailed(cmdList->Close());
    //ID3D12CommandList* ppCommandLists[] = { cmdList };
    //m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);
    
}

// 渲染，包含每帧的物理Update
void D3D12HelloConstBuffers::OnRender()
{
    UINT currentFrameIndex = m_frameIndex;
    UINT nextFrameIndex = (m_frameIndex + 1) % FrameCount;
    UINT lastFrameIndex = (m_frameIndex + 2) % FrameCount;

    // Execute当前帧，Present，并添加信号量
    frameResources[currentFrameIndex]->Execute(m_commandQueue.Get(), m_fenceValue);
    ThrowIfFailed(m_swapChain->Present(0, 0));
    m_frameIndex = (m_frameIndex + 1) % FrameCount;
    frameResources[currentFrameIndex]->Signal(m_commandQueue.Get(), m_fence.Get());

    // 更新下一帧的常量缓冲区，Populate下一帧的命令
    OnUpdate(*frameResources[nextFrameIndex], nextFrameIndex);
    PopulateCommandList(*frameResources[nextFrameIndex], nextFrameIndex);

    // Sync(等待)上一帧的执行。因为马上要更新它的常量缓冲区了，在这之前必须让它先执行好
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
    //IA：Input Assembler，OM：Output Merger，RS：Rasterize Stage

    //获取当前帧的信息
    frameRes.Populate();
    ID3D12GraphicsCommandList* cmdList = frameRes.cmdList.Get();
    ID3D12CommandAllocator* cmdAlloc = frameRes.cmdAllocator.Get();
    UploadBuffer* uploadBuffer = frameRes.perObjConstantUpload.get();
    ThrowIfFailed(cmdList->Reset(cmdAlloc, m_pipelineState.Get()));

    //之前创建好了根参数，由根参数组织成根签名。现在要设置根签名（根参数），后需要设置描述符堆，因为要把描述符堆和根参数联系起来
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
    cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    //绑定好根签名和描述符堆后，为每个根参数进行赋值（每个根形参都要用一个根实参来赋值）
    //给table实际赋值（table的根参数序号, BaseDescriptor）。因为在跟签名中已经设置了范围，这里只需要一个base就可以了
    //这里设置的是DefaultBuffeForConstant的Descriptor位置，所以offset是FrameCount
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
    cmdList->DrawInstanced(//如果有索引缓冲区，则需要使用DrawIndexedInstanced~松了口气吧~
        6, //VertexCountPerInstance：每个Instance要绘制的顶点数量。VertexCountPerInstance指定了被绘制的总顶点数，而绘制成什么样子由图元拓扑决定
        1, //InstanceCount：实例的数量。如果不想要“实例化”，设为1，然后把VertexCountPerInstance设为VertexBuffer的长度，StartVert设为零即可。如果想要，则可以看出，能随意定制啊！！
        0, //StartVertexLocation：顶点缓冲区内第一个被绘制的顶点的索引，即起始点
        0);//StartInstanceLocation：似上

    // Indicate that the back buffer will now be used to present.
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));

    ThrowIfFailed(cmdList->Close());
}