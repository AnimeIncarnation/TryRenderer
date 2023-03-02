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
#include "Resources/Model.h"
#include "Resources/BufferView.h"
#include "iostream"
#include <string>
#include <windowsx.h>

namespace rtti
{
    extern std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> GlobalLayout;
}

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
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
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
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

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
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;      //深度缓冲区和后台缓冲区是2D纹理です！！！
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
    }
}
//static XMFLOAT3 vertices[] = {
//    XMFLOAT3(0.5, -0.5, 0.5),
//    XMFLOAT3(-0.5, -0.5, 0.5),
//    XMFLOAT3(0.5, 0.5, 0.5),
//    XMFLOAT3(-0.5, 0.5, 0.5),
//    XMFLOAT3(0.5, 0.5, -0.5),
//    XMFLOAT3(-0.5, 0.5, -0.5),
//    XMFLOAT3(0.5, -0.5, -0.5),
//    XMFLOAT3(-0.5, -0.5, -0.5),
//    XMFLOAT3(0.5, 0.5, 0.5),
//    XMFLOAT3(-0.5, 0.5, 0.5),
//    XMFLOAT3(0.5, 0.5, -0.5),
//    XMFLOAT3(-0.5, 0.5, -0.5),
//    XMFLOAT3(0.5, -0.5, -0.5),
//    XMFLOAT3(0.5, -0.5, 0.5),
//    XMFLOAT3(-0.5, -0.5, 0.5),
//    XMFLOAT3(-0.5, -0.5, -0.5),
//    XMFLOAT3(-0.5, -0.5, 0.5),
//    XMFLOAT3(-0.5, 0.5, 0.5),
//    XMFLOAT3(-0.5, 0.5, -0.5),
//    XMFLOAT3(-0.5, -0.5, -0.5),
//    XMFLOAT3(0.5, -0.5, -0.5),
//    XMFLOAT3(0.5, 0.5, -0.5),
//    XMFLOAT3(0.5, 0.5, 0.5),
//    XMFLOAT3(0.5, -0.5, 0.5) 
//}; 
//static UINT indices[] = { 0, 2, 3
//, 0, 3, 1, 8, 4, 5, 8, 5, 9, 10, 6, 7, 10, 7, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23
//};


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
        //-0.1493827374, 0.97098, 0.1867284217
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
        ThrowIfFailed(cmdList->Reset(cmdAlloc.Get(), nullptr));
        //导入模型
        modelImporter = std::make_unique<ModelImporter>();
        modelImporter->Import("Models/nanosuit/nanosuit.obj");

        ////parse模型
        models.emplace_back(dxDevice.get());
        models.emplace_back(dxDevice.get());
        Model& model1 = models[0];
        Model& model2 = models[1];
        
        //model1.SetInputLayout(&rtti::Vertex::Instance(), 0, "Position4Normal3Color4");    //struct, slot = 0, layoutName
        //model1.ParseFromSpansAndUpload(cmdList.Get(), vertices, indices);
        model1.SetInputLayout(&rtti::Vertex2::Instance(), 0, "Position4Normal3TexCoord2");    //struct, slot = 0, layoutName
        model1.ParseFromAssimpAndUpload(cmdList.Get(), modelImporter.get());
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

        //CPU To Upload：Map + memcpy
        for (int i = 0;i < FrameCount;i++)
        {
            for (int j = 0;j < frameResources[i]->constantUpload.size();j++)
            {
                // 因为并不马上Unmap，所以并不适用UploadBuffer的CopyData成员函数，手动进行拷贝
                CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
                ThrowIfFailed(frameResources[i]->constantUpload[j]->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&frameResources[i]->mappedCbvData[j])));
                if(j==0)
                    memcpy(frameResources[i]->mappedCbvData[j], &cameraConstantData, frameResources[i]->constantBufferSize[j]);
                else if(j ==1)
                    memcpy(frameResources[i]->mappedCbvData[j], &lightConstantData, frameResources[i]->constantBufferSize[j]);

            }
        }

        //Upload To Default
        frameResources[m_frameIndex]->CopyConstantFromUploadToDefault();  //内含cmdList的操作

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
        
    //四、创建RasterShader，创建PSO（vs和ps的编译，RasterShader也记录PSO相关的光栅化信息）
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
        if(rtti::GlobalLayout.find("Position4Normal3Color4") != rtti::GlobalLayout.end())
            m_pipelineState = m_psoManager->GetPipelineState( //使用PSOManager创建了PipelineState
                dxDevice.get(),
                *colorShader,
                rtti::GlobalLayout["Position4Normal3Color4"],
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                rtvFmt,
                dsvFmt);
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
    //0. 暂时处理方法：相机的移动和帧数绑定
    mainCamera->Move();

    //1. 更新相机参数，获取更新后的相机参数，拷贝至map到的地址
    mainCamera->UpdateViewMatrix();
    mainCamera->UpdateProjectionMatrix();
    cameraConstantData.viewMatrix = mainCamera->GetViewMatrix();
    cameraConstantData.projMatrix = mainCamera->GetProjectionMatrix();
    cameraConstantData.vpMatrix = cameraConstantData.projMatrix * cameraConstantData.viewMatrix;
    memcpy(frameRes.mappedCbvData[0], &cameraConstantData, sizeof(cameraConstantData));

    //2. 更新灯光参数
    //sceneLights[0].Update();
    lightConstantData.direction = sceneLights[0].GetDirection();
    //sceneLights[1].Update();
    lightConstantData.position = sceneLights[1].GetPosition();
    memcpy(frameRes.mappedCbvData[1], &lightConstantData, sizeof(lightConstantData));

    //获取当前帧的信息
    ID3D12CommandAllocator* cmdAlloc = frameRes.cmdAllocator.Get();
    ID3D12GraphicsCommandList* cmdList = frameRes.cmdList.Get();

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
    for(auto& thisModel : models)
        frameRes.DrawMesh(dxDevice.get(), &thisModel, m_pipelineState.Get());           //设置PSO，设置IA，画
   

    // Indicate that the back buffer will now be used to present.
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)));
    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(m_depthTarget.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ)));
    ThrowIfFailed(cmdList->Close());
}

void Engine::OnKeyDown(UINT8 key)
{
    //因为Win32的WM_KEYDOWN检测频率太低，而且没有KeyHold这种选项，所以采用分发到OnUpdate的策略，这里仅控制状态机
    //std::cout << "OnKeyDown" << std::endl;
    //&8000是因为GetAsyncKeyState仅考虑第一位是否为1，为了让if判断能够支持这个语义，要&8000
    if (GetAsyncKeyState('W') & 0x8000)
    {
        mainCamera->isWalking = true;
        mainCamera->walkStep = 0.1f;
    }
    else if (GetAsyncKeyState('S') & 0x8000)
    {
        mainCamera->isWalking = true;
        mainCamera->walkStep = -0.1f;
    }
    if (GetAsyncKeyState('A') & 0x8000)//因为是position += strafeStep * right，所以A负D正
    {
        mainCamera->isStrafing = true;
        mainCamera->strafeStep = -0.1f;
    }
    else if (GetAsyncKeyState('D') & 0x8000)
    {
        mainCamera->isStrafing = true;
        mainCamera->strafeStep = 0.1f;
    }
    if (GetAsyncKeyState('E') & 0x8000)
    {
        mainCamera->isUpDown = true;
        mainCamera->upDownStep = 0.1f;
    }
    else if (GetAsyncKeyState('Q') & 0x8000)
    {
        mainCamera->isUpDown = true;
        mainCamera->upDownStep = -0.005f;
    }
}

void Engine::OnKeyUp(UINT8 key)
{
    //std::cout << "OnKeyUp" << std::endl;
    if (key == 'W')
    {
        mainCamera->isWalking = false;
    }
    else if (key == 'S')
    {
        mainCamera->isWalking = false;
    }
    if (key == 'A')
    {
        mainCamera->isStrafing = false;
    }
    else if (key == 'D')
    {
        mainCamera->isStrafing = false;
    }
    if (key == 'E')
    {
        mainCamera->isUpDown = false;
    }
    else if (key == 'Q')
    {
        mainCamera->isUpDown = false;
    }
    mainCamera->OutputPosition();
}

void Engine::OnMouseMove(WPARAM btnState, int x, int y)
{
    //std::cout << "OnMouseMove" << std::endl;
    //std::cout << "init: "
    //    << mainCamera->right.GetX() << " " << mainCamera->right.GetY() << " " << mainCamera->right.GetZ() << " "
    //    << mainCamera->up.GetX() << " " << mainCamera->up.GetY() << " " << mainCamera->up.GetZ() << " "
    //    << mainCamera->forward.GetX() << " " << mainCamera->forward.GetY() << " " << mainCamera->forward.GetZ() << std::endl;
    if ((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(
            0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(
            0.25f * static_cast<float>(y - mLastMousePos.y));
        mainCamera->Pitch(dy);
        mainCamera->RotateY(dx);
        mainCamera->OutputLookDir();
    }
    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

