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

#include "Common.hlsl"

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float3 worldPos : POSITION;
};

struct Cluster
{
    float3 points[8]; //近远然后从左上开始顺时针
    uint lightStart;
    uint lightCount;
};

struct PointLight   //点光源的包围盒
{
    float3 position;
    float radius;
    float3 strength; //color
};


ConstantBuffer<PerCameraConstants> PerCameraConstants : register(b0);
ConstantBuffer<SceneConstant> SceneConstant: register(b1);
StructuredBuffer<PointLight> AllPointLights: register(t1);
RWStructuredBuffer<Cluster> Clusters       : register(u4);
RWStructuredBuffer<uint> CulledPointLightIndices: register(u5);
RWStructuredBuffer<uint> LightAssignBuffer : register(u6); //存储的是CulledPointLightIndices的下标，相当于二级指针



float3 VPInverseTransform(float4x4 mat, float3 v3)
{
    float4 v4 = float4(v3, 1);
    v4 = mul(mat, v4);
    v4 /= v4.w;
    return v4.xyz;
}


float ViewDepth(float4 position)
{
    //return (2.0 * near * far) / (far + near - z * (far - near));

    float4 ndcPos = float4(position.x / 1280 * 2 - 1, (720 - position.y) / 720 * 2 - 1, position.z, 1);
    float4 viewPos = mul(PerCameraConstants.pInv, ndcPos);
    viewPos /= viewPos.w;
    return viewPos.z;
}

float3 PointShading(PointLight light, PSInput input)
{
    float3 vec = light.position - input.worldPos;
    float3 distance = length(vec);
    float3 irradiance = light.radius > distance ? light.strength * max(0, dot(normalize(vec), input.normal)) : float3(0, 0, 0);
    //float3 irradiance = light.radius > distance ? light.strength : float3(0, 0, 0);
    return irradiance;
}


[earlydepthstencil]
float4 main(PSInput input) : SV_TARGET
{
    ////拿到片元所属Cluster
    float viewZ = ViewDepth(input.position);
    float ZRatio = (viewZ - SceneConstant.nearZ) / (SceneConstant.farZ - SceneConstant.nearZ); 
    uint3 clusterIndex = uint3(floor((input.position.x) / 1280 * 32), floor((720-(input.position.y)) / 720 * 16), floor(ZRatio * 128));
    if (clusterIndex.x < 32 && clusterIndex.y < 16 && clusterIndex.z < 128)
    {
        Cluster currCluster = Clusters[clusterIndex.x + clusterIndex.y * 32 + clusterIndex.z * 32 * 16];
        //input.color = float4(float(currCluster.lightCount)/6, float(currCluster.lightCount)/6, float(currCluster.lightCount)/6, 1);

        //遍历影响该Cluster的光源并计算光照
        for (uint count = 0;count < currCluster.lightCount; count++)
        {
          uint interID = LightAssignBuffer[currCluster.lightStart + count];
          uint id = CulledPointLightIndices[interID];
          PointLight light = AllPointLights[id];
          float3 shadingRes = PointShading(light, input);
          input.color += float4(shadingRes, 0);
        }
        input.color += float4(0, 0, 0, 1);
    }
    return input.color;
}
