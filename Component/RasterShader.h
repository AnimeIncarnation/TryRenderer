#pragma once
#ifndef _RASTERSHADER_H_
#define _RASTERSHADER_H_

#include "Shader.h"
using Microsoft::WRL::ComPtr;

//DXIL。仅获取编译好的Shader。在Build期间会自动编译好Shader的到cso文件
struct CompiledShaderObject
{
	byte* data;
	UINT size;
};


class RasterShader: public Shader
{
public:
	//这里仅可能存储已经compile过的shader
	CompiledShaderObject vsShader;
	CompiledShaderObject psShader;
	CompiledShaderObject hsShader;
	CompiledShaderObject dsShader;
	CompiledShaderObject msShader;
	CompiledShaderObject asShader;
	D3D12_RASTERIZER_DESC rasterizeState;
	D3D12_DEPTH_STENCIL_DESC depthStencilState;
	D3D12_BLEND_DESC blendState;
	RasterShader(std::span<std::pair<std::string, Parameter> const> properties, DXDevice* device);
	RasterShader(std::span<std::pair<std::string, Parameter> const> properties, ComPtr<ID3D12RootSignature>&& rootSig);
};
#endif

