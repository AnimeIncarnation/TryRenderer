#pragma once
#ifndef _CLUSTER_LIGHTING_
#define _CLUSTER_LIGHTING_

#include "../DXMath/MathHelper.h"
#include "../Resources/UploadBuffer.h"
#include "../Resources/DefaultBuffer.h"

struct Cluster
{
	Math::XMFLOAT3 points[8];
	UINT lightStart;
	UINT lightCount;

};

struct PointLight
{
	Math::XMFLOAT3 position;
	float radius;
	Math::XMFLOAT3 strength; //color
};

class ClusterLighting
{
	//CPU - AllPointLights
	std::vector<PointLight> scenePointLights;

	//AllPointLights
	std::unique_ptr<UploadBuffer> pointLightUpload;
	void* mappedUploadData;

	//AllPointLights + Clusters + CulledPointLightIndices + LightAssignBuffer
	std::unique_ptr<DefaultBuffer> pointLightDefault; 
	std::unique_ptr<DefaultBuffer> clusterBuffer; 
	std::unique_ptr<DefaultBuffer> culledPointLightBuffer; 
	std::unique_ptr<DefaultBuffer> lightAssignBuffer; 



	UINT maxLightCountPerCluster = 8;
public:
	void InsertPointLight(const PointLight&);
	
	//插入结束才可Create和Copy，因为根据light vector的长度来决定create和copy的size
	void CreateBuffers(DXDevice* dxdevice, UINT64 clusterBufferSize);
	void CopyAndUpload(ID3D12GraphicsCommandList* cmdList);

	UINT GetPointLightCount()const { return scenePointLights.size(); }
	UINT GetMaxLightCountPerCluster()const { return maxLightCountPerCluster; }
	void SetMaxLightCountPerCluster(UINT c) { maxLightCountPerCluster = c; }

	D3D12_GPU_VIRTUAL_ADDRESS GetPointLightBufferAddress()const 
	{ 
		return pointLightDefault->GetGPUAddress(); 
	}
	D3D12_GPU_VIRTUAL_ADDRESS GetClusterBufferAddress()const { return clusterBuffer->GetGPUAddress(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetCulledPointLightBufferAddress()const { return culledPointLightBuffer->GetGPUAddress(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetLightAssignBufferAddress()const 
	{ 
		return lightAssignBuffer->GetGPUAddress(); 
	}
};


#endif

