#pragma once
#ifndef _RASTERSHADER_H_
#define _RASTERSHADER_H_

#include "Shader.h"
using Microsoft::WRL::ComPtr;
class RasterShader: public Shader
{
public:
	//这里仅可能存储已经compile过的shader
	ComPtr<ID3DBlob> vsShader = nullptr;
	ComPtr<ID3DBlob> psShader = nullptr;
	ComPtr<ID3DBlob> hsShader = nullptr;
	ComPtr<ID3DBlob> dsShader = nullptr;
	D3D12_RASTERIZER_DESC rasterizeState;
	D3D12_DEPTH_STENCIL_DESC depthStencilState;
	D3D12_BLEND_DESC blendState;
	RasterShader(std::span<std::pair<std::string, Parameter> const> properties, DXDevice* device);
	RasterShader(std::span<std::pair<std::string, Parameter> const> properties, ComPtr<ID3D12RootSignature>&& rootSig);
};
#endif

