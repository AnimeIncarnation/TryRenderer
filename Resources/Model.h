#pragma once
#ifndef _MODEL_H_
#define _MODEL_H_

#include "Resource.h"
#include "DefaultBuffer.h"
#include "UploadBuffer.h"
#include "../Utilities/VertexStructDescriber.h"
#include "../RenderBase/ModelImporter.h"

class Model : public Resource 
{
	const rtti::ElementStruct * vertexStruct;		//该model所使用的layout对应的vertex结构体实例
	std::pair<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> layout;	//Mesh要存储根据顶点元信息parse出的layout
	UINT vertexLayoutSlot;
	std::vector<DefaultBuffer> vertexPerMesh;
	std::vector<DefaultBuffer> indexPerMesh;
	std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;
	std::vector<D3D12_INDEX_BUFFER_VIEW> indexBufferView;
	std::vector<UploadBuffer> uploadBufferVertex;
	std::vector<UploadBuffer> uploadBufferIndex;
	std::vector<UINT64> vertexCount;
	std::vector<UINT64> indiceCount;
	bool parsed = false;
public:
	Model(DXDevice* device);
	void SetInputLayout(const rtti::ElementStruct* vbStruct, UINT slot, const std::string& layoutName);
	void ParseFromAssimpAndUpload(ID3D12GraphicsCommandList* cmdList, ModelImporter* modelImporter);
	void ParseFromSpansAndUpload(ID3D12GraphicsCommandList* cmdList, std::span<DirectX::XMFLOAT3 const> vertices, std::span<UINT const> indices);
	std::span<const DefaultBuffer> VertexBuffers() const { return vertexPerMesh; }
	std::span<const DefaultBuffer> IndexBuffer() const { return indexPerMesh; }
	std::span<const UINT64> GetVertexCount()const { return vertexCount; }
	std::span<const UINT64> GetIndiceCount()const { return indiceCount; }
	std::span<const D3D12_INPUT_ELEMENT_DESC> Layout() const { return layout.second; }
	const std::vector<D3D12_VERTEX_BUFFER_VIEW>& GetVertexBufferView()const;
	const std::vector<D3D12_INDEX_BUFFER_VIEW>& GetIndexBufferView()const;
};




#endif