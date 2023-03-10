#include "PSOManager.h"
#include <iostream>

ID3D12PipelineState* PSOManager::GetPipelineState(std::string name)
{
	if (PSOMap.find(name) != PSOMap.end())
		return PSOMap[name].Get();
	return nullptr;
}



ID3D12PipelineState* PSOManager::GetSetPipelineState(
	DXDevice* dxDevice,
	const RasterShader& rasterShader,
	std::span<const D3D12_INPUT_ELEMENT_DESC> inputLayout,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType,
	std::span<DXGI_FORMAT> rtvFormats,
	DXGI_FORMAT dsvFormat,
	std::string name)
{
	//如果Map中已经有了，则直接返回PSO
	if (PSOMap.find(name) != PSOMap.end())
		return PSOMap[name].Get();
	//否则创建PSO并插入Map
	struct PSO_STREAM
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_AS AS;
		CD3DX12_PIPELINE_STATE_STREAM_MS MS;
		//CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
		CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendState;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencilState;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;
	} Stream;

	Stream.RootSignature = rasterShader.GetRootSignature();
	Stream.AS = { rasterShader.asShader.data, rasterShader.asShader.size };
	Stream.MS = { rasterShader.msShader.data, rasterShader.msShader.size };
	//Stream.VS = {rasterShader.vsShader.data, rasterShader.vsShader.size};
	Stream.PS = { rasterShader.psShader.data, rasterShader.psShader.size };
	Stream.RasterizerState = CD3DX12_RASTERIZER_DESC(rasterShader.rasterizeState);
	Stream.BlendState = CD3DX12_BLEND_DESC(rasterShader.blendState);
	Stream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(rasterShader.depthStencilState);
	//Stream.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };	 //如果使用MeshShader则不能使用InputLayout
	Stream.PrimitiveTopologyType = topologyType;
	Stream.DSVFormat = dsvFormat;
	D3D12_RT_FORMAT_ARRAY rtvFormatArray;
	rtvFormatArray.NumRenderTargets = std::min<UINT>(rtvFormats.size(), 8);
	for (UINT i = 0;i < 8;i++)
	{
		if(i >= rtvFormatArray.NumRenderTargets)
			rtvFormatArray.RTFormats[i] = DXGI_FORMAT_UNKNOWN;
		else
			rtvFormatArray.RTFormats[i] = rtvFormats[i];
	}
	Stream.RTVFormats = rtvFormatArray;
	//超采样相关直接硬编码
	Stream.SampleMask = UINT_MAX;
	DXGI_SAMPLE_DESC sampleDesc;
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;
	Stream.SampleDesc = sampleDesc;

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	streamDesc.pPipelineStateSubobjectStream = &Stream;
	streamDesc.SizeInBytes = sizeof(Stream);

	ID3D12PipelineState* spPso;
	ThrowIfFailed(dxDevice->Device()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&spPso)));
	PSOMap.insert({ name, spPso });
	return spPso;
}
