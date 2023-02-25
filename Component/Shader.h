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

//Shader基类含有：shader参数（即根签名），
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
	std::unordered_map<std::string, InsideParameter> parameters;
	std::optional<InsideParameter> GetParameter(std::string_view name) 
	{
		auto ite = parameters.find(std::string(name));
		if (ite == parameters.end()) return {};
		return ite->second;	
	}
	
public:
	Shader(std::span<std::pair<std::string, Parameter>const>, DXDevice*); //并不是要把device移动过来，而是仅仅用一下它。是裸指针而不是右值引用
	Shader(std::span<std::pair<std::string, Parameter>const>, ComPtr<ID3D12RootSignature>&&); //这里要把根签名移动进来让Shader管理了
	bool SetParameter(ID3D12GraphicsCommandList*, std::string, CD3DX12_GPU_DESCRIPTOR_HANDLE); //写入根实参之描述符的句柄（二级指针）（根参数为CBV/SRV/UAV TABLE）
	bool SetParameter(ID3D12GraphicsCommandList*, std::string, D3D12_GPU_VIRTUAL_ADDRESS); //写入根实参之缓冲区地址（一级指针）（根参数为CBV/SRV/UAV）
	ID3D12RootSignature* GetRootSignature() const { return rootSignature.Get(); }
	Shader(Shader&&) = default;
};

#endif
