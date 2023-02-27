#pragma once
#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#include "../DXMath/MathHelper.h"


//CBufferҪ256B���룬ͬʱ���밴��HLSL�ķ�װ�����ԡ���䡱�����ķ�ʽ������C++�ṹ�壬������ȷmemcpy
//����ÿ������Ա֮��ľ��������4����������������Ϊcbuffer������ݴ��Ϊ4D����
struct PerLightConstant
{
	Math::XMFLOAT3 stength;
    float fallOffStart;
	Math::XMFLOAT3 position;
    float fallOffEnd;
    Math::XMFLOAT3 direction;
    float spotPower;
    char padding[208];
};
static_assert((sizeof(PerLightConstant) % 256) == 0, "Constant Buffer size must be 256-byte aligned");
static constexpr UINT PerLightConstantBufferSize = sizeof(PerLightConstant);    // CB size is required to be 256-byte aligned.


struct PerCameraConstant
{
    Math::Matrix4 viewMatrix;
    Math::Matrix4 projMatrix;
    Math::Matrix4 vpMatrix;
    char padding[64];
};
static_assert((sizeof(PerCameraConstant) % 256) == 0, "Constant Buffer size must be 256-byte aligned");
static constexpr UINT PerCameraConstantBufferSize = sizeof(PerCameraConstant);    // CB size is required to be 256-byte aligned.

struct PerMaterialConstant
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.5f;
    // Used in texture mapping.
    DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
    UINT DiffuseMapIndex = 0;
    UINT NormalMapIndex = 0;
    char padding[152];
};
static_assert((sizeof(PerMaterialConstant) % 256) == 0, "Constant Buffer size must be 256-byte aligned");
static constexpr UINT PerMaterialConstantBufferSize = sizeof(PerMaterialConstant);    // CB size is required to be 256-byte aligned.


struct SuperCameraConstant
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProjTex = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ShadowTransform = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;
    DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
    char padding[192];

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    //Light Lights[MaxLights];
};
static_assert((sizeof(SuperCameraConstant) % 256) == 0, "Constant Buffer size must be 256-byte aligned");
static constexpr UINT SuperCameraConstantBufferSize = sizeof(SuperCameraConstant);    // CB size is required to be 256-byte aligned.


#endif

