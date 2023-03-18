#pragma once
#ifndef _SHADER_PARAMETER_TYPE_H_
#define _SHADER_PARAMETER_TYPE_H_

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