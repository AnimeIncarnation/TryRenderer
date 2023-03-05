//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************



struct PerCameraConstants
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 vpMatrix;
};
ConstantBuffer<PerCameraConstants> perCameraConstants : register(b0);


struct PerLightConstant
{
    float3 stength;
    float fallOffStart;
    float3 position;
    float fallOffEnd;
    float3 direction;
    float spotPower;
};
ConstantBuffer<PerLightConstant> perLightConstants: register(b1);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};


float4 main(PSInput input) : SV_TARGET
{
    float3 backDir = -perLightConstants.direction;
    float res = max(0, dot(input.normal, backDir));
    input.color = float4(res, res, res, 1);
    return input.color;
}
