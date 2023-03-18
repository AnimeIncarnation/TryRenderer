#include "Common.hlsl"


//Cluster Renderingÿ֡��ɢٷ�Cluster���ڵ��Դ����׶�޳����۵��Դ��ϸ��׶����(Cluster����)
//������clusters��ע��points��ÿ���̴߳���һ��cluster
//�ڰ��±�ע��culledPointLightIndices��ÿ���̴߳���һ�����Դ�����Ե��Դ����������С��max(meshletCount * instanceCount, clusterNum)
//�۰�lightStart��lightCountע��clusters��Ȼ������lightAssignBuffer��ÿ���̴߳���һ��cluster���о����У���Ϊ����������Ҫ�Ⱥ�˳��ģ�
//������Ҫ������֮�����԰����ŵ�MeshShader����



struct Cluster
{
    float3 points[8]; //��ԶȻ������Ͽ�ʼ˳ʱ��
    uint lightStart;
    uint lightCount;
};

struct PointLight   //���Դ�İ�Χ��
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
    
    //���磨xy�ڴ˷ָz�ڴ���ȫ������������ռ�ָ
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

    //2. ��Դ����׶�޳�����ÿ���ƹ����Ƿ�����׶���ڣ��������
    uint dtID_1D = groupID * 32 * 16 + groupThreadID.x + groupThreadID.y * 32;
    if (dtID_1D < SceneConstant.pointLightCount)
    {
        PointLight currPointLight = AllPointLights[dtID_1D];
        for (uint i = 0; i < 6; ++i)
        {
            //�㵽ƽ����빫ʽ
            if (dot(float4(currPointLight.position,1), PerCameraConstants.frustum[i]) >= currPointLight.radius)
            {
                //��¼һ��ֵ����ʾ�ƹⲻ����׶����
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