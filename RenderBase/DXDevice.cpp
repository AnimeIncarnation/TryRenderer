#include "../stdafx.h"
#include "../DXSample.h"
#include "DXDevice.h"

DXDevice::DXDevice()
{
    //1. 开启Debug Layer
    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        // Enable additional debug layers.
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    //2. 创建Factory
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    //3. 创建Adaptor
    UINT adapterIndex = 0;
    while (factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        //如果拿到的是Intel的显卡而非我的Amphere架构显卡，则跳过
        if (desc.Description[0] == 'I')
        {
            adapterIndex++;
            continue;
        }
        //4. 创建device
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) 
        {
            HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2,IID_PPV_ARGS(&device));
            D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureData = {};
            device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureData, sizeof(featureData));
            if (featureData.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
                std::cout << "do not support mesh shader" << std::endl;
            if (SUCCEEDED(hr)) 
            {
                break;
            }
        }
        adapter = nullptr;
        adapterIndex++;
    }
}

DXDevice::~DXDevice()
{

}