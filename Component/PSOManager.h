#pragma once
#ifndef _PSOMANAGER_H_
#define _PSOMANAGER_H_

#include "RasterShader.h"
class PSOManager
{
	//RasterShader shader;
	std::unordered_map<D3D12_GRAPHICS_PIPELINE_STATE_DESC*, ComPtr<ID3D12PipelineState>> PSOMap;

public:
	ID3D12PipelineState* GetPipelineState(DXDevice* dxDevice, D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc);
	ID3D12PipelineState* GetPipelineState(	//����PSO�����г�ԱSampleMask�ͳ�ԱSampleDesc.Count��Ӳ����Ϊ1
		DXDevice* dxDevice,
		const RasterShader& rasterShader,	//Shaders + RasterizerState + BlendState + DepthStencilState
		std::span<const D3D12_INPUT_ELEMENT_DESC> inputLayout,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType,
		std::span<DXGI_FORMAT> rtvFormats,
		DXGI_FORMAT dsvFormat
	);
	//std::optional<ID3D12PipelineState*> GetPipelineState(DXDevice* dxDevice, D3D12_GRAPHICS_PIPELINE_STATE_DESC& stateDesc);

};
#endif
