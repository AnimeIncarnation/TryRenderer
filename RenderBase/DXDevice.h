#pragma once
#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "../stdafx.h"
using Microsoft::WRL::ComPtr;

class DXDevice
{
private:
	ComPtr<IDXGIFactory4> factory;
	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<ID3D12Device8> device;

public:
	ComPtr<IDXGIFactory4> Factory() const { return factory.Get(); }
	ComPtr<IDXGIAdapter1> Adapter() const { return adapter.Get(); }
	ComPtr<ID3D12Device8> Device() const { return device.Get(); }

public:
	DXDevice();
	~DXDevice();
	DXDevice(const DXDevice& copy) = delete;
	DXDevice operator=(const DXDevice& copy) = delete;

};


#endif