#include "ComputeShader.h"

ComputeShader::ComputeShader(std::span<std::pair<std::string, Parameter>const> properties, DXDevice* device)
	: Shader(properties, device)
{
}

ComputeShader::ComputeShader(std::span<std::pair<std::string, Parameter>const> properties, ComPtr<ID3D12RootSignature>&& rootSig)
	: Shader(properties, std::move(rootSig))
{
}
