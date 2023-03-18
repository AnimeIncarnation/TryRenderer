
#include "Common.hlsl"

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


ConstantBuffer<SceneConstant> SceneConstant: register(b1);
StructuredBuffer<PointLight> AllPointLights: register(t0);
RWStructuredBuffer<Cluster> Clusters       : register(u0);
RWStructuredBuffer<uint> CulledPointLightIndices: register(u1);
RWStructuredBuffer<uint> LightAssignBuffer : register(u2);//存储的是CulledPointLightIndices的下标，相当于二级指针



bool IsIntersect(Cluster cluster, PointLight light)
{
    if (distance(cluster.points[0], light.position) < light.radius ||
        distance(cluster.points[1], light.position) < light.radius ||
        distance(cluster.points[2], light.position) < light.radius ||
        distance(cluster.points[3], light.position) < light.radius ||
        distance(cluster.points[4], light.position) < light.radius ||
        distance(cluster.points[5], light.position) < light.radius ||
        distance(cluster.points[6], light.position) < light.radius ||
        distance(cluster.points[7], light.position) < light.radius)
        return true;
    return false;
}


//Cluster获取它拥有的点光源。一个线程处理一个Cluster
[NumThreads(32, 16, 1)]
void main(
    uint3 groupThreadID : SV_GroupThreadID,
    uint groupID : SV_GroupID)
{
    uint dtID_1D = groupID * 32 * 16 + groupThreadID.x + groupThreadID.y * 32;
    uint assignBufferStartPosition = dtID_1D * SceneConstant.maxLightCountPerCluster;
    uint count = 0;

    for (uint i = 0;i < SceneConstant.pointLightCount;i++)
    {
        uint lightIndice = CulledPointLightIndices[i];
        if (lightIndice == SceneConstant.pointLightCount + 1)
            continue;

        PointLight currLight = AllPointLights[lightIndice];
        if (!IsIntersect(Clusters[dtID_1D], currLight))
            continue;
        LightAssignBuffer[assignBufferStartPosition + count] = i;
        count++;
        if (count == SceneConstant.maxLightCountPerCluster)
            break;
    }
    Clusters[dtID_1D].lightCount = count;
    Clusters[dtID_1D].lightStart = assignBufferStartPosition;
 
}