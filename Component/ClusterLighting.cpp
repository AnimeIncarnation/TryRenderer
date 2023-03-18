#include "ClusterLighting.h"
#include "../DXSampleHelper.h"


void ClusterLighting::InsertPointLight(const PointLight& pointLight)
{
	scenePointLights.emplace_back(pointLight);
}

void ClusterLighting::CopyAndUpload(ID3D12GraphicsCommandList* cmdList)
{

	pointLightUpload->CopyData(0, { reinterpret_cast<const byte*>(scenePointLights.data()) ,scenePointLights.size() * sizeof(PointLight) });
	cmdList->CopyBufferRegion(pointLightDefault->GetResource(), 0, pointLightUpload->GetResource(), 0, scenePointLights.size() * sizeof(PointLight));
	// 目前只开头提交一次
	//cmdList->ResourceBarrier(1,
	//	get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(
	//		pointLightDefault->GetResource(),
	//		D3D12_RESOURCE_STATE_COPY_DEST,
	//		D3D12_RESOURCE_STATE_COMMON)));

}

void ClusterLighting::CreateBuffers(DXDevice* dxdevice, UINT64 clusterBufferSize)
{
	UINT pointLightCount = scenePointLights.size();
	pointLightUpload = std::make_unique<UploadBuffer>(dxdevice, pointLightCount * sizeof(PointLight));
	pointLightDefault = std::make_unique<DefaultBuffer>(dxdevice, pointLightCount * sizeof(PointLight));
	clusterBuffer = std::make_unique<DefaultBuffer>(dxdevice, clusterBufferSize);
	culledPointLightBuffer = std::make_unique<DefaultBuffer>(dxdevice, pointLightCount * sizeof(UINT));
	lightAssignBuffer = std::make_unique<DefaultBuffer>(dxdevice, 32 * 16 * 128 * maxLightCountPerCluster * sizeof(UINT));
}
