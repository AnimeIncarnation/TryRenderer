#pragma once
#ifndef _SHADER_H_
#define _SHADER_H_

#include <unordered_map>
#include <optional>
#include <span>
#include "ShaderParameterType.h"
#include "../stdafx.h"
#include "../DXSampleHelper.h"
#include "../RenderBase/DXDevice.h"


struct CompiledShaderObject
{
	byte* data;
	UINT size;
};


//Shader������Ҫ����shader����������ǩ����
class Shader
{
public:
	struct Parameter
	{
		ShaderParameterType type;
		UINT registerIndex;
		UINT spaceIndex;
		UINT arraySize;
	};

protected:
	struct InsideParameter : Parameter
	{
		UINT rootSigIndex;
		InsideParameter(Parameter const& p, UINT index): Parameter(p),rootSigIndex(index) {}
	};
	ComPtr<ID3D12RootSignature> rootSignature;
	std::vector<std::pair<std::string, InsideParameter>> parameters;
	std::optional<InsideParameter> GetParameter(std::string_view name) 
	{
		for(auto& iter : parameters)
			if (iter.first == name) 
				return iter.second;
		return {};
	}
	
public:
	Shader(std::span<std::pair<std::string, Parameter>const>, DXDevice*); //������Ҫ��device�ƶ����������ǽ�����һ����������ָ���������ֵ����
	Shader(std::span<std::pair<std::string, Parameter>const>, ComPtr<ID3D12RootSignature>&&); //����Ҫ�Ѹ�ǩ���ƶ�������Shader������
	bool SetParameter(ID3D12GraphicsCommandList*, std::string, CD3DX12_GPU_DESCRIPTOR_HANDLE); //д���ʵ��֮�������ľ��������ָ�룩��������ΪCBV/SRV/UAV TABLE��
	bool SetParameter(ID3D12GraphicsCommandList*, std::string, D3D12_GPU_VIRTUAL_ADDRESS); //д���ʵ��֮��������ַ��һ��ָ�룩��������ΪCBV/SRV/UAV��
	bool SetComputeParameter(ID3D12GraphicsCommandList* cmdList, std::string name, D3D12_GPU_VIRTUAL_ADDRESS address);
	ID3D12RootSignature* GetRootSignature() const { return rootSignature.Get(); }
	Shader(Shader&&) = default;
};

#endif
