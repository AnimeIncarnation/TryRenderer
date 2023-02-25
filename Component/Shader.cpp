#include "Shader.h"

Shader::Shader(std::span<std::pair<std::string, Parameter>const> params, DXDevice* dxDevice)
{
    //1. 注入哈希表
    for (int i = 0;i < params.size();i++)
    {
        InsideParameter iParam = InsideParameter((params.begin() + i)->second,i);
        parameters.insert_or_assign((params.begin() + i)->first, iParam);
    }

    //2. 建立根参数表
    std::vector<CD3DX12_DESCRIPTOR_RANGE1> allRanges;
    std::vector<CD3DX12_ROOT_PARAMETER1> allRootParameters;
    size_t rangeIter = 0;
    for (auto& param : parameters)
    {
        auto iParam = param.second;
        switch (iParam.type)
        {
        case ShaderParameterType::ConstantBufferView:
            allRootParameters.emplace_back().InitAsConstantBufferView(iParam.registerIndex, iParam.spaceIndex);
            break;
        case ShaderParameterType::ShaderResourceView:
            allRootParameters.emplace_back().InitAsShaderResourceView(iParam.registerIndex, iParam.spaceIndex);
            break;
        case ShaderParameterType::UnorderedAccessView:
            allRootParameters.emplace_back().InitAsUnorderedAccessView(iParam.registerIndex, iParam.spaceIndex);
            break;
        case ShaderParameterType::CBVTable:
            allRanges.emplace_back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, iParam.arraySize == 0 ? 1 : iParam.arraySize, iParam.registerIndex, iParam.spaceIndex);
            allRootParameters.emplace_back().InitAsDescriptorTable(1, &allRanges[rangeIter]);
            rangeIter++;
            break;
        case ShaderParameterType::SRVTable:
            allRanges.emplace_back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, iParam.arraySize == 0 ? 1 : iParam.arraySize, iParam.registerIndex, iParam.spaceIndex);
            allRootParameters.emplace_back().InitAsDescriptorTable(1, &allRanges[rangeIter]);
            rangeIter++;
            break;
        case ShaderParameterType::UAVTable:
            allRanges.emplace_back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, iParam.arraySize == 0 ? 1 : iParam.arraySize, iParam.registerIndex, iParam.spaceIndex);
            allRootParameters.emplace_back().InitAsDescriptorTable(1, &allRanges[rangeIter]);
            rangeIter++;
            break;
        }

    }
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
        allRootParameters.size(), allRootParameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    //D3D12指定必须先将根签名的描述符布局进行序列化处理，转换为以blob接口表示的序列化数据之后才可传入CreateRootSignature中
    ID3D12Device* device = dxDevice->Device().Get();
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(dxDevice->Device()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
    ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

Shader::Shader(std::span<std::pair<std::string, Parameter>const>params, ComPtr<ID3D12RootSignature>&& sig):rootSignature(std::move(sig))
{
    //注入哈希表
    for (int i = 0;i < params.size();i++)
    {
        InsideParameter iParam = InsideParameter((params.begin() + i)->second, i);
        parameters.insert_or_assign((params.begin() + i)->first, iParam);
    }
}

bool Shader::SetParameter(ID3D12GraphicsCommandList* cmdList, std::string name, CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
{
    auto var = GetParameter(name);
    if (!var) 
        return false;
    UINT paramIndex = var->rootSigIndex;
    cmdList->SetGraphicsRootDescriptorTable(paramIndex, handle);
	return true;
}

bool Shader::SetParameter(ID3D12GraphicsCommandList* cmdList, std::string name, D3D12_GPU_VIRTUAL_ADDRESS address)
{
    auto var = GetParameter(name);
    if (!var)
        return false;
    UINT paramIndex = var->rootSigIndex;
    switch (var->type)
    {
    case ShaderParameterType::ConstantBufferView:
        cmdList->SetGraphicsRootConstantBufferView(var->rootSigIndex, address);
        break;
    case ShaderParameterType::ShaderResourceView:
        cmdList->SetGraphicsRootShaderResourceView(var->rootSigIndex, address);
        break;
    case ShaderParameterType::UnorderedAccessView:
        cmdList->SetGraphicsRootUnorderedAccessView(var->rootSigIndex, address);
        break;
    default:
        return false;
    }
	return true;
}
