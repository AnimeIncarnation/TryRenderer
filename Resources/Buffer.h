#pragma once
#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "Resource.h"

class Buffer :public Resource
{
public:
	Buffer(DXDevice* device) :Resource(device) {}
	Buffer(Buffer&&) = default;
	virtual ~Buffer() = default;
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const = 0;
	virtual UINT64 GetSize() const = 0;
};



#endif