
#pragma once
#ifndef _COMPUTE_SHADER_H_
#define _COMPUTE_SHADER_H_

#include "Shader.h"
using Microsoft::WRL::ComPtr;

//DXIL������ȡ����õ�Shader����Build�ڼ���Զ������Shader�ĵ�cso�ļ�


class ComputeShader : public Shader
{
public:
	//��������ܴ洢�Ѿ�compile����shader
	std::vector<CompiledShaderObject> csShader;
	ComputeShader(std::span<std::pair<std::string, Parameter> const> properties, DXDevice* device);
	ComputeShader(std::span<std::pair<std::string, Parameter> const> properties, ComPtr<ID3D12RootSignature>&& rootSig);
};
#endif

