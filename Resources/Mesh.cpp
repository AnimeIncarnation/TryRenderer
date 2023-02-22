#include "Mesh.h"

Mesh::Mesh(
	DXDevice* device, 
	std::span<const rtti::ElementStruct*> vbStructs, 
	UINT64 vertexCount)
	:Resource(device),vertexCount(vertexCount), vertexStructs(vbStructs)
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
		//�����ǵ�DefaultBuffer�õ�VertexBufferView��View/Span������ʼ��ַ��size��stride
		res.BufferLocation = v.GetGPUAddress();
		res.SizeInBytes = v.GetSize();
		res.StrideInBytes = res.SizeInBytes / vertexCount;
	}
}