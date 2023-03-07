#pragma once
#ifndef _INSTANCECONTROLLER_H_
#define _INSTANCECONTROLLER_H_

#include <minwindef.h>
#include "../DXMath/MathHelper.h"
#include "../Resources/UploadBuffer.h"
#include "../Resources/DefaultBuffer.h"

class InstanceController
{
	struct PerObjectInstanceData
	{
		Math::Matrix4 worldMatrix;
	};
	DXDevice* dxdevice;
	UINT row;
	float radius;		//间隔/2
	UINT instanceCount;
	UINT instanceDataSize;
	std::vector<PerObjectInstanceData> perInstanceData;
	std::unique_ptr<UploadBuffer> instanceUpload ;
	std::unique_ptr<DefaultBuffer>instanceDefault;
	std::unique_ptr<UploadBuffer> drawParamUpload ;
	std::unique_ptr<DefaultBuffer>drawParamDefault;
	
public:
	InstanceController(DXDevice* device, UINT row, float radius) :
		dxdevice(device), row(row), instanceCount(row* row* row), radius(radius) {}

	UINT GetInstanceCount() const { return instanceCount; }
	void GeneratePerInstanceDataAndUpload(ID3D12GraphicsCommandList* cmdList)
	{
		instanceDataSize = instanceCount * sizeof(PerObjectInstanceData);
		//生成实例数据
		for (UINT i = 0; i < instanceCount; ++i)
		{
			XMVECTOR index = XMVectorSet(float(i % row), float((i / row) % row), float(i / (row * row)), 0);
			XMVECTOR location = index * radius * 2 - XMVectorReplicate(radius * row);

			XMMATRIX world = XMMatrixTranslationFromVector(location);
			PerObjectInstanceData& data = perInstanceData.emplace_back();
			data.worldMatrix.SetX(world.r[0]);
			data.worldMatrix.SetY(world.r[1]);
			data.worldMatrix.SetZ(world.r[2]);
			data.worldMatrix.SetW(world.r[3]);
		}
		
		instanceUpload = std::make_unique<UploadBuffer>(dxdevice, instanceDataSize);
		instanceUpload->CopyData(0, { reinterpret_cast<const byte*>(perInstanceData.data()) ,instanceDataSize});
		instanceDefault= std::make_unique<DefaultBuffer>(dxdevice, instanceDataSize);
		cmdList->CopyBufferRegion(instanceDefault.get()->GetResource(), 0, instanceUpload.get()->GetResource(), 0, instanceDataSize);

		drawParamUpload = std::make_unique<UploadBuffer>(dxdevice, sizeof(UINT) * 2);
		UINT arr[2] = { instanceCount, 0 };
		drawParamUpload->CopyData(0, { reinterpret_cast<const byte*>(arr) ,sizeof(UINT) * 2 });
		drawParamDefault = std::make_unique<DefaultBuffer>(dxdevice, sizeof(UINT) * 2);
		cmdList->CopyBufferRegion(drawParamDefault.get()->GetResource(), 0, drawParamUpload.get()->GetResource(), 0, sizeof(UINT) * 2);
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetDrawParamsGPUAddress() const { return drawParamDefault->GetGPUAddress(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetInstanceDataGPUAddress() const
	{
		return instanceDefault->GetGPUAddress();
	}
};


#endif
