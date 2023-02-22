#pragma once
#ifndef _MESH_H_
#define _MESH_H_

#include "Resource.h"
#include "DefaultBuffer.h"
#include "../Utilities/VertexStructDescriber.h"

//Mesh是GPU上对于顶点的DefaultResources封装
class Mesh : public Resource 
{
	std::vector<DefaultBuffer> vertexBuffers;//一种顶点类型对应一个DefaultBuffer
	UINT64 vertexCount;
	std::span<const rtti::ElementStruct *> vertexStructs; //实例化顶点结构体时获得的顶点元信息（们）
	std::vector<D3D12_INPUT_ELEMENT_DESC> layout; //Mesh要存储根据顶点元信息parse出的layout（们）

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