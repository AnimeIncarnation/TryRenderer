#include "Mesh.h"

Mesh::Mesh(
	DXDevice* device, 
	std::span<const rtti::ElementStruct*> vbStructs, 
	UINT64 vertexCount,
	UINT64 indiceCount)
	:Resource(device), indexBuffer(device, sizeof(uint)* indiceCount), vertexCount(vertexCount),indiceCount(indiceCount), vertexStructs(vbStructs)
{
	//vertexBuffers.reserve(vertexStructs.size());
	uint slotCount = 0;
	for (auto&& x:vertexStructs)
	{
		vertexBuffers.emplace_back(dxdevice, vertexCount * x->structSize);
		x->GetMeshLayout(slotCount,layout);
		++slotCount;
	}
}

void Mesh::GetVertexBufferView(std::vector<D3D12_VERTEX_BUFFER_VIEW>& result) const
{
	result.clear();
	result.resize(vertexBuffers.size());
	for (size_t i = 0; i < vertexBuffers.size(); ++i) {
		D3D12_VERTEX_BUFFER_VIEW& res = result[i];
		const DefaultBuffer& v = vertexBuffers[i];
		//从我们的DefaultBuffer得到VertexBufferView：View/Span含有起始地址，size和stride
		res.BufferLocation = v.GetGPUAddress();
		res.SizeInBytes = v.GetSize();
		res.StrideInBytes = res.SizeInBytes / vertexCount;
	}
}

D3D12_INDEX_BUFFER_VIEW Mesh::GetIndexBufferView() const
{
	D3D12_INDEX_BUFFER_VIEW res;
	const DefaultBuffer& v = IndexBuffer();
	res.BufferLocation = v.GetGPUAddress();
	res.SizeInBytes = v.GetSize();
	res.Format = DXGI_FORMAT_R32_UINT;
	return res;
}
