#pragma once
#ifndef _RESOURCES_H_
#define _RESOURCES_H_

#include "../stdafx.h"
#include "../RenderBase/DXDevice.h"
using Microsoft::WRL::ComPtr;

//��Դ�ࣺ�����ƶ�����ֹ����
class Resource
{
protected:
	//Resource��Ҫ����һ�����������豸
	DXDevice* dxdevice;

public:
	DXDevice* GetDXDevice() { return dxdevice; }
	Resource(DXDevice* d):dxdevice(d) {}
	Resource(const Resource&) = delete;
	Resource(Resource&&) = default;
	virtual ~Resource() = default;
	virtual ID3D12Resource* GetResource() const {return nullptr;}
	virtual D3D12_RESOURCE_STATES GetResourceState() const {return D3D12_RESOURCE_STATE_COMMON;}
};



#endif