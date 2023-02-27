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
#define _CRT_SECURE_NO_WARNINGS
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
    cameraConstantData{},
    lightConstantData{}
{
}

void Engine::OnInit()
{
    LoadPipeline();
    LoadAssets();
}

// 创建命令队列，交换链，RTV和CBV的描述符堆，并在RTV堆中创建交换链的描述符
void Engine::LoadPipeline()
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
  
    //二、创建描述符堆（RTV堆，CBV堆，DSV堆），创建多个RTV
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
            cbvHeapDesc.NumDescriptors = FrameCount * 2 * 2; //FrameCount, Default/Upload代表2, 再乘以cb的种类（PerCamera, PerLight）
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
        // 2. 在堆中创建RTV
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
   
    //三、创建深度缓冲区和DSV
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

    //四、创建帧资源（常量缓冲区也在此创建）
    {
        for (auto&& i : frameResources)
        {
            i = std::make_unique<FrameResource>(dxDevice.get());
            i->EmplaceConstantBuffer(dxDevice.get(), PerCameraConstantBufferSize);
            i->EmplaceConstantBuffer(dxDevice.get(), PerLightConstantBufferSize);
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


// 创建根签名相关内容，VS，PS，PSO，命令列表，同步用fence
void Engine::LoadAssets()
{
    auto& cmdList = frameResources[m_frameIndex]->cmdList;
    auto& cmdAlloc = frameResources[m_frameIndex]->cmdAllocator;

    //零、建立相机，建立场景灯光
    {
        mainCamera = std::make_unique<Camera>();
        Math::Vector3 up = Math::Vector3(-0.3181089, 0.8988663, 0.301407);
        Math::Vector3 forward = Math::Vector3(-0.6525182, -0.438223, 0.6182076);
        Math::Vector3 pos = Math::Vector3(2.232773, 1.501817, -1.883978);
        mainCamera->LookAt(pos, forward, up);
        mainCamera->aspect = static_cast<float>(m_scissorRect.right) / static_cast<float>(m_scissorRect.bottom);
        mainCamera->UpdateViewMatrix();
        mainCamera->UpdateProjectionMatrix();

        Light& parallelLight = sceneLights.emplace_back(Light::LightType::PARALLEL);
        Light& pointLight = sceneLights.emplace_back(Light::LightType::POINT);
    }

    //一、建立Mesh（建立存储顶点的Resource）：
    //①获取顶点和索引数据
    //②创建UploadBuffer，把数据从CPU端拷贝到GPU端。
    //③创建Mesh，把数据从UploadBuffer拷贝到Mesh中的DefaultBuffer。
    {   
        modelImporter = std::make_unique<ModelImporter>();
        modelImporter->Import("Models/nanosuit/nanosuit.obj");
        UINT vertexCount = modelImporter->GetModels()[0].vertices.size();
        UINT indiceCount = modelImporter->GetModels()[0].indices.size();
        //UINT vertexCount = array_count(vertices);
        //UINT indiceCount = array_count(indices);



        //创建字节数组作为CPU端VertexBuffer，把Assimp得到的数据parse过来
        std::vector<byte> modelVertices(Vertex::Instance().structSize * vertexCount);
        byte* vertexBytePtr = modelVertices.data();
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        for (int i = 0;i < vertexCount;i++)
        {
            XMFLOAT4 position = { modelImporter->GetModels()[0].vertices[i].position, 1 };
            //position.y -= 14;
            Vertex::Instance().position.Get(vertexBytePtr) = position;
            XMFLOAT3 normal = modelImporter->GetModels()[0].vertices[i].normal;
            Vertex::Instance().normal.Get(vertexBytePtr) = normal;
            XMFLOAT4 color(position.x + 0.5f, position.y + 0.5f, position.z + 0.5f, 1);
            Vertex::Instance().color.Get(vertexBytePtr) = color;
            vertexBytePtr += Vertex::Instance().structSize;
            /*XMFLOAT4 position = { vertices[i], 1 };
            Vertex::Instance().position.Get(vertexBytePtr) = position;
            XMFLOAT3 normal = { 0,0,0 };
            Vertex::Instance().normal.Get(vertexBytePtr) = normal;
            XMFLOAT4 color(position.x + 0.5f, position.y + 0.5f, position.z + 0.5f, 1);
            Vertex::Instance().color.Get(vertexBytePtr) = color;
            vertexBytePtr += Vertex::Instance().structSize;*/
            //std::cout << position.x <<" "<< position.y << " " << position.z << std::endl;
            {
                Math::Matrix4 vpMatrix = mainCamera->GetProjectionMatrix() * mainCamera->GetViewMatrix();
                //Math::XMFLOAT4 position = vpMatrix* Math::XMFLOAT4{ vertices[i], 1 };
                Math::XMFLOAT4 position = vpMatrix* Math::XMFLOAT4{ modelImporter->GetModels()[0].vertices[i].position, 1 };
                std::cout << position.x << " "<< position.y << " "<< position.z <<" "<<position.w << std::endl;
            }
        }

        //创建UploadBuffer，并输送顶点数据到UploadBuffer
        uploadBufferVertex = std::make_unique<UploadBuffer>(dxDevice.get(), modelVertices.size());
        uploadBufferIndex = std::make_unique<UploadBuffer>(dxDevice.get(), indiceCount * sizeof(UINT));
        uploadBufferVertex->CopyData(0, modelVertices);  //offset为0，数据从cubeVertices中拷贝
        uploadBufferIndex->CopyData(0, { reinterpret_cast<byte const*>(&modelImporter->GetModels()[0].indices), indiceCount * sizeof(UINT) });  //offset为0，数据从cubeVertices中拷贝
        //uploadBufferIndex->CopyData(0, { reinterpret_cast<byte const*>(indices), indiceCount * sizeof(UINT) });  //offset为0，数据从cubeVertices中拷贝


        //创建Mesh，把UploadBuffer数据拷贝到DefaultBuffer
        std::vector<const rtti::ElementStruct *> structs;
        structs.emplace_back(&Vertex::Instance());
        cubeMesh = std::make_unique<Mesh>(dxDevice.get(), structs, vertexCount, indiceCount);
        ID3D12Resource* vertexDefault = cubeMesh->VertexBuffers()[0].GetResource();
        ID3D12Resource* indexDefault = cubeMesh->IndexBuffer().GetResource();

        ThrowIfFailed(cmdList->Reset(cmdAlloc.Get(), nullptr));
        cmdList->CopyBufferRegion(vertexDefault, 0, uploadBufferVertex->GetResource(), 0, uploadBufferVertex->GetSize());
        cmdList->CopyBufferRegion(indexDefault, 0, uploadBufferIndex->GetResource(), 0, uploadBufferIndex->GetSize());
    }

    //二、给每个帧资源创建常量缓冲区描述符，并进行数据初始化（Upload + Default结构）：
    {
        //开始创建常量缓冲区——要先创建View
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
        for (auto&& i : frameResources)
        {        
            for (int j = 0;j < i->constantUpload.size();j++)
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
                cbvDesc.BufferLocation = i->constantUpload[j]->GetGPUAddress();
                cbvDesc.SizeInBytes = i->constantBufferSize[j];
                dxDevice->Device()->CreateConstantBufferView(&cbvDesc, cbvHandle);
                cbvHandle.Offset(1, m_cbvDescriptorSize);

                cbvDesc.BufferLocation = i->constantDefault[j]->GetGPUAddress();
                cbvDesc.SizeInBytes = i->constantBufferSize[j];
                dxDevice->Device()->CreateConstantBufferView(&cbvDesc, cbvHandle);
                cbvHandle.Offset(1, m_cbvDescriptorSize);
            }
        }


        //拷贝内存中的常量数据到Upload常量缓冲区在内存中的映射中：Map + memcpy
        for (int i = 0;i < FrameCount;i++)
        {
            for (int j = 0;j < frameResources[i]->constantUpload.size();j++)
            {
                // 因为并不马上Unmap，所以并不适用UploadBuffer的成员函数，手动进行拷贝
                CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
                ThrowIfFailed(frameResources[i]->constantUpload[j]->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&frameResources[i]->mappedCbvData[j])));
                if(j==0)
                    memcpy(frameResources[i]->mappedCbvData[j], &cameraConstantData, frameResources[i]->constantBufferSize[j]);
                else if(j ==1)
                    memcpy(frameResources[i]->mappedCbvData[j], &lightConstantData, frameResources[i]->constantBufferSize[j]);

            }
        }

        //把UploadBuffer数据拷贝到DefaultBuffer
        //for (auto& frame : frameResources)
        //{
        frameResources[m_frameIndex]->CopyConstantFromUploadToDefault();  //内含cmdList的操作
        //}

        //Execute命令列表，这里不用同步，等LoadAssets函数结束的时候一起同步
        ThrowIfFailed(cmdList->Close());
        ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
        m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);
    }

    //三、创建Shader参数（即根签名）（带有一个描述符表，表中带有一个CBV）：
    {
        std::vector<std::pair<std::string, Shader::Parameter>> shaderParams;
        shaderParams.emplace_back("PerCameraConstant", Shader::Parameter{ 
            ShaderParameterType::ConstantBufferView,
            0,      //registerIndex
            0 });      //spaceIndex
        shaderParams.emplace_back("PerLightConstant", Shader::Parameter{
            ShaderParameterType::ConstantBufferView,
            1,      //registerIndex
            0 });         //spaceIndex
            //1//arraySize(numDescriptors)(only for tables)
        colorShader = std::make_unique<RasterShader>(shaderParams, dxDevice.get());
    }
        
    //四、创建RasterShader（vs和ps的编译，RasterShader也记录PSO相关的光栅化信息）
    {
        m_psoManager = std::make_unique<PSOManager>();
        //使用RasterShader的信息构建pipelineState
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        //注：使用GetAssetFullPath时Throw错误，直接写入shaders.hlsl后正常运行
        ThrowIfFailed(D3DCompileFromFile(std::wstring(L"Shaders/shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(std::wstring(L"Shaders/shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, nullptr));
        colorShader->vsShader = vertexShader;
        colorShader->psShader = pixelShader;
        colorShader->rasterizeState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        colorShader->blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        colorShader->depthStencilState.DepthEnable = TRUE;
        colorShader->depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        colorShader->depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        colorShader->depthStencilState.StencilEnable = FALSE;
        std::vector<DXGI_FORMAT> rtvFmt;
        rtvFmt.push_back(DXGI_FORMAT_R8G8B8A8_UNORM);
        DXGI_FORMAT dsvFmt = DXGI_FORMAT_D32_FLOAT;
        m_pipelineState = m_psoManager->GetPipelineState( //使用PSOManager创建了PipelineState
            dxDevice.get(),
            *colorShader,
            cubeMesh->Layout(),
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            rtvFmt,
            dsvFmt);

        //// Describe and create the graphics pipeline state object (PSO).
        //D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};                            //管线状态对象包括：
        //psoDesc.pRootSignature = m_rootSignature.Get();                             //根签名（shader参数）
        //psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());                   //VS。DX会自动检测VS的输入和“顶点结构体布局”是否匹配，后者一定要>=前者（意会）
        //psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());                    //PS
        //psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);           //光栅状态（硬件支持的MSAA？背面剔除？线框模式？阴影bias？保守光栅化？）
        //psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                     //混合状态（混合方式？是否开启AlphaToCoverage？MRT时的多混合状态？）
        //psoDesc.DepthStencilState.DepthEnable = TRUE;                               //深度写入√
        //psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;           //小于则写入
        //psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        //psoDesc.DepthStencilState.StencilEnable = FALSE;                            //模板测试？
        //psoDesc.InputLayout = { cubeMesh->Layout().data(),
        //                       cubeMesh->Layout().size() };                        //顶点结构体布局。用自定义的Mesh类获取
        //psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;     //拓扑类型（点/线/三角形/Patch）。如果制定了HS和DS，必须为Patch
        //psoDesc.NumRenderTargets = 1;                                               //RenderTarget的数量：即RTVFormats数组长度
        //psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                         //RenderTarget的模式：每个RenderTarget的格式
        //psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        //psoDesc.SampleMask = UINT_MAX;                                              //多重采样采样点允许与否的掩码。32位bit
        //psoDesc.SampleDesc.Count = 1;                                               //指定多重采样的采样数量和质量级别。数量必须与渲染目标的对应数据相同
        //ThrowIfFailed(dxDevice->Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
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
void Engine::OnUpdate(FrameResource& frameRes, UINT64 frameIndex)
{
    //更新相机参数，获取更新后的相机参数，拷贝至map到的地址
    mainCamera->UpdateViewMatrix();
    mainCamera->UpdateProjectionMatrix();
    cameraConstantData.viewMatrix = mainCamera->GetViewMatrix();
    cameraConstantData.projMatrix = mainCamera->GetProjectionMatrix();
    cameraConstantData.vpMatrix = cameraConstantData.projMatrix * cameraConstantData.viewMatrix;
    memcpy(frameRes.mappedCbvData[0], &cameraConstantData, sizeof(cameraConstantData));

    //更新灯光参数
    //sceneLights[0].Update();
    lightConstantData.direction = sceneLights[0].GetDirection();
    //sceneLights[1].Update();
    lightConstantData.position = sceneLights[1].GetPosition();
    memcpy(frameRes.mappedCbvData[1], &lightConstantData, sizeof(lightConstantData));

    //获取当前帧的信息
    ID3D12CommandAllocator* cmdAlloc = frameRes.cmdAllocator.Get();
    ID3D12GraphicsCommandList* cmdList = frameRes.cmdList.Get();
    //UploadBuffer* currUpload = frameRes.constantUpload.get();

    //PS：由于使用DefaultBuffer做常量缓冲区时关闭程序会莫名报错，而且渲染好像也变慢了，所以还是使用UploadBuffer
    //每次更新常量缓冲区数据时，把UploadBuffer数据拷贝到DefaultBuffer
    ThrowIfFailed(cmdAlloc->Reset());
    ThrowIfFailed(cmdList->Reset(cmdAlloc, m_pipelineState.Get()));

    //把UploadBuffer数据拷贝到DefaultBuffer
    frameRes.CopyConstantFromUploadToDefault();  //内含cmdList的操作


    //这里不Execute，等和populate一起执行。不然会出错，必须要同步才行

    //ThrowIfFailed(cmdList->Close());
    //ID3D12CommandList* ppCommandLists[] = { cmdList };
    //m_commandQueue->ExecuteCommandLists(array_count(ppCommandLists), ppCommandLists);
}

// 渲染，包含每帧的物理Update
void Engine::OnRender()
{
    UINT currentFrameIndex = m_frameIndex;
    UINT nextFrameIndex = (m_frameIndex + 1) % FrameCount;

    // Execute当前帧，Present，并添加信号量
    frameResources[currentFrameIndex]->Execute(m_commandQueue.Get(), m_fenceValue);
    ThrowIfFailed(m_swapChain->Present(0, 0));
    m_frameIndex = (m_frameIndex + 1) % FrameCount;
    frameResources[currentFrameIndex]->Signal(m_commandQueue.Get(), m_fence.Get());

    // Sync(等待)下一帧的执行。因为马上要更新它的常量缓冲区了，在这之前必须让它先执行好
    frameResources[nextFrameIndex]->Sync(m_fence.Get());

    // 更新下一帧的常量缓冲区，Populate下一帧的命令
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
    // IA（Input Assembler）：1. 图元拓扑详细版  2. 顶点缓冲区  3. 索引缓冲区
    // OM（Output Merger）：设置渲染目标RenderTarget
    // RS（Rasterize Stage）：1. 设置视口  2. 设置裁剪矩形
    // Shader参数相关：1. 设置根签名  2. 设置描述符堆  3. 注入根实参
    // 其他：针对RT和DS的Clear操作，设置PSO

    // 要手写的只有Shader参数相关的三个——①设置描述符堆，②设置根签名，③注入根实参

    //获取当前帧的信息
    frameRes.Populate();
    ID3D12GraphicsCommandList* cmdList = frameRes.cmdList.Get();
    ID3D12CommandAllocator* cmdAlloc = frameRes.cmdAllocator.Get();

    //1. 设置描述符堆和根签名
    ////之前创建好了根参数，由根参数组织成根签名。现在要设置根签名（根参数），后需要设置描述符堆，因为要把描述符堆和根参数联系起来
    cmdList->SetGraphicsRootSignature(colorShader->GetRootSignature());
    ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
    cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    //2. 注入根实参
    //绑定好根签名和描述符堆后，为每个根参数进行赋值（每个根形参都要用一个根实参来赋值）
    //给table实际赋值（table的根参数序号, BaseDescriptor）。因为在跟签名中已经设置了范围，这里只需要一个base就可以了
    //这里设置的是DefaultBuffeForConstant的Descriptor位置，所以offset是FrameCount
    //CD3DX12_GPU_DESCRIPTOR_HANDLE cameraCbvHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), frameIndex * 4 + 1, m_cbvDescriptorSize);
    //CD3DX12_GPU_DESCRIPTOR_HANDLE lightsCbvHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart(), frameIndex * 4 + 3, m_cbvDescriptorSize);
    //colorShader->SetParameter(cmdList, "PerCameraConstant", cameraCbvHandle);
    //colorShader->SetParameter(cmdList, "PerLightConstant", lightsCbvHandle);
    D3D12_GPU_VIRTUAL_ADDRESS cameraCbAddress = frameRes.constantDefault[0]->GetGPUAddress();
    D3D12_GPU_VIRTUAL_ADDRESS lightCbAddress = frameRes.constantDefault[1]->GetGPUAddress();
    colorShader->SetParameter(cmdList, "PerCameraConstant", cameraCbAddress);
    colorShader->SetParameter(cmdList, "PerLightConstant", lightCbAddress);

    // Indicate that the back buffer will be used as a render target.
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)));
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_depthTarget.Get(), D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE)));
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_dsvDescriptorSize);
    frameRes.SetRenderTarget(&m_viewport, &m_scissorRect, &rtvHandle, &dsvHandle);  //设置RS和OM
    frameRes.ClearRenderTarget(rtvHandle);                                          //清除RT
    frameRes.ClearDepthStencilBuffer(dsvHandle);                                    //清除DS
    frameRes.DrawMesh(                                                              //设置PSO，设置IA，画
        dxDevice.get(),             //拿到设备
        colorShader.get(),          //拿到Shader
        m_psoManager.get(),         //拿到pso
        cubeMesh.get(),             //拿到mesh
        DXGI_FORMAT_R8G8B8A8_UNORM, //RTVのFormat
        DXGI_FORMAT_D32_FLOAT);     //DSVのFormat
   
    //std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;
    //cubeMesh->GetVertexBufferView(vertexBufferView);
    //cmdList->IASetVertexBuffers(0, vertexBufferView.size(), vertexBufferView.data());
    //D3D12_INDEX_BUFFER_VIEW indexBufferView = cubeMesh->GetIndexBufferView();
    //cmdList->IASetIndexBuffer(&indexBufferView);
    //cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);      //更细化的图元拓扑：列表/条带......
    //cmdList->DrawIndexedInstanced(cubeMesh->GetIndiceCount(), 1, 0, 0, 0);
    ////cmdList->DrawInstanced(//如果有索引缓冲区，则需要使用DrawIndexedInstanced~松了口气吧~
    ////    array_count(vertices), //VertexCountPerInstance：每个Instance要绘制的顶点数量。VertexCountPerInstance指定了被绘制的总顶点数，而绘制成什么样子由图元拓扑决定
    ////    1, //InstanceCount：实例的数量。如果不想要“实例化”，设为1，然后把VertexCountPerInstance设为VertexBuffer的长度，StartVert设为零即可。如果想要，则可以看出，能随意定制啊！！
    ////    0, //StartVertexLocation：顶点缓冲区内第一个被绘制的顶点的索引，即起始点
    ////    0);//StartInstanceLocation：似上

    // Indicate that the back buffer will now be used to present.
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_depthTarget.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ)));
    ThrowIfFailed(cmdList->Close());
}