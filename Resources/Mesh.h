#pragma once
#ifndef _MESH_H_
#define _MESH_H_

#include "Resource.h"
#include "DefaultBuffer.h"
#include "../Utilities/VertexStructDescriber.h"

//Mesh��GPU�϶��ڶ����DefaultResources��װ
class Mesh : public Resource 
{
	std::vector<DefaultBuffer> vertexBuffers;//һ�ֶ������Ͷ�Ӧһ��DefaultBuffer
	UINT64 vertexCount;
	std::span<const rtti::ElementStruct *> vertexStructs; //ʵ��������ṹ��ʱ��õĶ���Ԫ��Ϣ���ǣ�
	std::vector<D3D12_INPUT_ELEMENT_DESC> layout; //MeshҪ�洢���ݶ���Ԫ��Ϣparse����layout���ǣ�

public:
	Mesh(
		DXDevice* device,
		std::span<const rtti::ElementStruct*> vbStructs,
		UINT64 vertexCount);
	std::span<const DefaultBuffer> VertexBuffers() const { return vertexBuffers; }
	std::span<const D3D12_INPUT_ELEMENT_DESC> Layout() const { return layout; }
	void GetVertexBufferView(std::vector<D3D12_VERTEX_BUFFER_VIEW>& result) const;
	UINT64 GetVertexCount()const { return vertexCount; }
};

#endif