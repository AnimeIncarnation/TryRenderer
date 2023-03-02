#include "PSOManager.h"
#include <iostream>

ID3D12PipelineState* PSOManager::GetPipelineState(DXDevice* dxDevice, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	auto returnValue = PSOMap.try_emplace(&psoDesc);
	if (returnValue.second)	   //如果成功插入
		ThrowIfFailed(dxDevice->Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&returnValue.first->second)));
	return returnValue.first->second.Get();
}

ID3D12PipelineState* PSOManager::GetPipelineState(
	DXDevice* dxDevice,
	const RasterShader& rasterShader,
	std::span<const D3D12_INPUT_ELEMENT_DESC> inputLayout,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType,
	std::span<DXGI_FORMAT> rtvFormats,
	DXGI_FORMAT dsvFormat)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC  psoDesc = {};	//这里一定要写上={}，否则报错
	psoDesc.pRootSignature = rasterShader.GetRootSignature();
	auto GetByteCode = [](const ComPtr<ID3DBlob>& shaderCode) -> D3D12_SHADER_BYTECODE {
		if (shaderCode)
			return CD3DX12_SHADER_BYTECODE(shaderCode.Get());
		else
			return CD3DX12_SHADER_BYTECODE(nullptr, 0); //shaderCode*, byteLength
	};
	psoDesc.VS = GetByteCode(rasterShader.vsShader.Get());
	psoDesc.PS = GetByteCode(rasterShader.psShader.Get());
	psoDesc.DS = GetByteCode(rasterShader.dsShader.Get());
	psoDesc.HS = GetByteCode(rasterShader.dsShader.Get());//CD3DX12_SHADER_BYTECODE((void*)0xcccccccc,(size_t)3435973836);
	psoDesc.RasterizerState = rasterShader.rasterizeState;
	psoDesc.BlendState = rasterShader.blendState;
	psoDesc.DepthStencilState = rasterShader.depthStencilState;
	psoDesc.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };
	psoDesc.PrimitiveTopologyType = topologyType;
	psoDesc.DSVFormat = dsvFormat;
	UINT rtvSize = std::min<UINT>(rtvFormats.size(), 8);
	psoDesc.NumRenderTargets = rtvSize;
	for (UINT i = 0;i < rtvSize;i++)
		psoDesc.RTVFormats[i] = rtvFormats[i];
	//超采样相关直接硬编码
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;

	auto returnValue = PSOMap.try_emplace(&psoDesc);
	if (returnValue.second)	   //如果成功插入
		ThrowIfFailed(dxDevice->Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&returnValue.first->second)));
	else
		std::cout << "fuck" <<std::endl;
	return returnValue.first->second.Get();
}
