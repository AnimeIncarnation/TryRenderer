#pragma once
#ifndef _RASTERSHADER_H_
#define _RASTERSHADER_H_

#include "Shader.h"
using Microsoft::WRL::ComPtr;

//DXIL������ȡ����õ�Shader����Build�ڼ���Զ������Shader�ĵ�cso�ļ�
struct CompiledShaderObject
{
	byte* data;
	UINT size;
};


class RasterShader: public Shader
{
public:
	//��������ܴ洢�Ѿ�compile����shader
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

