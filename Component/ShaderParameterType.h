#pragma once
#ifndef _SHADERPARAMETERTYPE_H_
#define _SHADERPARAMETERTYPE_H_

enum class ShaderParameterType
{
	ConstantBufferView,		//ConstantBuffer
	ShaderResourceView,		//StructuredBuffer
	UnorderedAccessView,	//RWStructuredBuffer
	CBVTable,
	SRVTable,
	UAVTable,
};
#endif