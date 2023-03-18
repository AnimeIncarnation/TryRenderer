#include "Common.hlsl"


//Cluster Rendering每帧完成①分Cluster，②点光源的视锥剔除，③点光源的细视锥分配(Cluster分配)
//①先在clusters中注入points。每个线程处理一个cluster
//②把下标注入culledPointLightIndices。每个线程处理一个点光源，所以点光源的数量必须小于max(meshletCount * instanceCount, clusterNum)
//③把lightStart和lightCount注入clusters，然后生成lightAssignBuffer。每个线程处理一个cluster（感觉不行，因为这三部是需要先后顺序的，
//三必须要在两者之后，所以把三放到MeshShader？）



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


ConstantBuffer<PerCameraConstants> PerCameraConstants   : register(b0);
ConstantBuffer<SceneConstant> SceneConstant: register(b1);
StructuredBuffer<PointLight> AllPointLights: register(t0);
RWStructuredBuffer<Cluster> Clusters       : register(u0);
RWStructuredBuffer<uint> CulledPointLightIndices: register(u1);


float3 VPInverseTransform(float4x4 mat, float3 v3)
{
    float4 v4 = float4(v3, 1);
    v4 = mul(mat, v4);
    v4 /= v4.w;
    return v4.xyz;
}


[NumThreads(32, 16, 1)]
void main(
    uint3 groupThreadID : SV_GroupThreadID,
    uint groupID : SV_GroupID)
{

    //NDC
    float x_min = (float(groupThreadID.x)/32) * 2 - 1;
    float x_max = (float(groupThreadID.x+1)/32) * 2 - 1;
    float y_min = (float(groupThreadID.y)/16) * 2 - 1;
    float y_max = (float(groupThreadID.y+1)/16) * 2 - 1;
    float z_min = float(groupID)/128;
    float z_max = float(groupID+1)/128;
    
    //世界（xy在此分割，z在此做全长，等做世界空间分割）
    float3 points[8];
    points[0] = VPInverseTransform(PerCameraConstants.vpInv, float3(x_min, y_min, 0));
    points[1] = VPInverseTransform(PerCameraConstants.vpInv, float3(x_max, y_min, 0));
    points[2] = VPInverseTransform(PerCameraConstants.vpInv, float3(x_min, y_max, 0));
    points[3] = VPInverseTransform(PerCameraConstants.vpInv, float3(x_max, y_max, 0));
    points[4] = VPInverseTransform(PerCameraConstants.vpInv, float3(x_min, y_min, 1));
    points[5] = VPInverseTransform(PerCameraConstants.vpInv, float3(x_max, y_min, 1));
    points[6] = VPInverseTransform(PerCameraConstants.vpInv, float3(x_min, y_max, 1));
    points[7] = VPInverseTransform(PerCameraConstants.vpInv, float3(x_max, y_max, 1));

    uint clusterIndex = groupThreadID.x + groupThreadID.y * 32 + groupID * 32 * 16;
    Clusters[clusterIndex].points[0] = points[0] + (points[4] - points[0]) * z_min;
    Clusters[clusterIndex].points[1] = points[1] + (points[5] - points[1]) * z_min;
    Clusters[clusterIndex].points[2] = points[2] + (points[6] - points[2]) * z_min;
    Clusters[clusterIndex].points[3] = points[3] + (points[7] - points[3]) * z_min;
    Clusters[clusterIndex].points[4] = points[0] + (points[4] - points[0]) * z_max;
    Clusters[clusterIndex].points[5] = points[1] + (points[5] - points[1]) * z_max;
    Clusters[clusterIndex].points[6] = points[2] + (points[6] - points[2]) * z_max;
    Clusters[clusterIndex].points[7] = points[3] + (points[7] - points[3]) * z_max;

    //2. 光源的视锥剔除。对每个灯光检查是否在视锥体内，是则加入
    uint dtID_1D = groupID * 32 * 16 + groupThreadID.x + groupThreadID.y * 32;
    if (dtID_1D < SceneConstant.pointLightCount)
    {
        PointLight currPointLight = AllPointLights[dtID_1D];
        for (uint i = 0; i < 6; ++i)
        {
            //点到平面距离公式
            if (dot(float4(currPointLight.position,1), PerCameraConstants.frustum[i]) >= currPointLight.radius)
            {
                //记录一个值来表示灯光不在视锥体内
                CulledPointLightIndices[dtID_1D] = SceneConstant.pointLightCount+1;
                break;
            }
            if (i == 5)
            {
                CulledPointLightIndices[dtID_1D] = dtID_1D;
            }
        }
    }
}