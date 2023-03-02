#pragma once
#ifndef _TEXTURE_H_
#define _TEXTURE_H_
#include "Resource.h"
#include "../RenderBase/DXDevice.h"

class Texture: Resource
{
	ComPtr<ID3D12Resource> resource;
	UINT64 size;

public:
	Texture(DXDevice* device) :Resource(device) {}
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture(Texture&&) = default;


};

#endif

