#pragma once
#ifndef _PSOMANAGER_H_
#define _PSOMANAGER_H_

#include "RasterShader.h"
class PSOManager
{
	//RasterShader shader;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSOMap;

public:
	ID3D12PipelineState* GetPipelineState(std::string name);

	ID3D12PipelineState* GetSetPipelineState(	//创建PSO。其中成员SampleMask和成员SampleDesc.Count都硬编码
		DXDevice* dxDevice,
		const RasterShader& rasterShader,	//Shaders + RasterizerState + BlendState + DepthStencilState
		std::span<const D3D12_INPUT_ELEMENT_DESC> inputLayout,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType,
		std::span<DXGI_FORMAT> rtvFormats,
		DXGI_FORMAT dsvFormat,
		std::string name
	);
	//std::optional<ID3D12PipelineState*> GetPipelineState(DXDevice* dxDevice, D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc);

};
#endif
