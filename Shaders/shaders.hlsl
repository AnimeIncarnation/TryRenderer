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
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;
    //result.position = position;
    //float4 p;
    //p.x = position.x; p.y = position.y; p.z = position.z; p.w = 1;
    result.position = mul(perCameraConstants.vpMatrix, position);
    result.position.y -= 30;
    result.position.z += 6;
    result.position.w += 6;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
