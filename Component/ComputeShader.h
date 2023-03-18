
#pragma once
#ifndef _COMPUTE_SHADER_H_
#define _COMPUTE_SHADER_H_

#include "Shader.h"
using Microsoft::WRL::ComPtr;

//DXIL。仅获取编译好的Shader。在Build期间会自动编译好Shader的到cso文件


class ComputeShader : public Shader
{
public:
	//这里仅可能存储已经compile过的shader
	std::vector<CompiledShaderObject> csShader;
	ComputeShader(std::span<std::pair<std::string, Parameter> const> properties, DXDevice* device);
	ComputeShader(std::span<std::pair<std::string, Parameter> const> properties, ComPtr<ID3D12RootSignature>&& rootSig);
};
#endif

