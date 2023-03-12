
struct Payload
{
    uint MeshletIndices[32];
    uint InstanceIndices[32];
};

struct InstanceInfo
{
    uint InstanceCount;
    uint InstanceOffset;
};

struct MeshInfo
{
    uint MeshletCount;
};

struct PerCameraConstants
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 vpMatrix;
    float4 frustum[6];
};

struct PerInstanceData
{
    float4x4 modelMatrix;
};

struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimitiveCount;
    uint PrimitiveOffset;
    float3 boundingBox[2];	//min, max
    float4 boundingSphere;	//pos, r
};




// ע��Payload��groupshared���ݣ���������һ��SM(Group)�ڵ�����
groupshared Payload payload;
ConstantBuffer<PerCameraConstants>  perCameraConstants  : register(b0);
ConstantBuffer<InstanceInfo>        instanceInfo        : register(b2);
ConstantBuffer<MeshInfo>            meshInfo            : register(b3);
RWStructuredBuffer<Meshlet>         Meshlets            : register(u0);
StructuredBuffer<PerInstanceData>   InstanceData        : register(t0);



bool IsOutsideFrustum(uint meshletId, uint instanceId)
{
    Meshlet currMeshlet = Meshlets[meshletId];
    float4x4 instanceModelMatrix = InstanceData[instanceId].modelMatrix;

    // ����BoundingSphere����һ�δ���׶�޳�
    float4 center = mul(instanceModelMatrix, float4(currMeshlet.boundingSphere.xyz, 1));
    float radius = currMeshlet.boundingSphere.w;
    for (uint i = 0; i < 6; ++i)
    {
        //����ǰ�ߵ�ʱ��frustum[0]��w���ں���
        if (dot(center, perCameraConstants.frustum[i]) >= radius) 
        {
            return true;
        }
    }

    //��AABB��׶�޳�
    float3 worldBB[2] = 
    { 
        mul(instanceModelMatrix, float4(currMeshlet.boundingBox[0],1)).xyz ,
        mul(instanceModelMatrix, float4(currMeshlet.boundingBox[1],1)).xyz 
    };
    float3 points[8] = 
    {
        float3(worldBB[0].x, worldBB[0].y, worldBB[0].z),
        float3(worldBB[1].x, worldBB[0].y, worldBB[0].z),
        float3(worldBB[0].x, worldBB[1].y, worldBB[0].z),
        float3(worldBB[0].x, worldBB[0].y, worldBB[1].z),
        float3(worldBB[1].x, worldBB[1].y, worldBB[0].z),
        float3(worldBB[1].x, worldBB[0].y, worldBB[1].z),
        float3(worldBB[0].x, worldBB[1].y, worldBB[1].z),
        float3(worldBB[1].x, worldBB[1].y, worldBB[1].z) 
    };
    for (uint i = 0;i < 6;i++)
    {
        for (uint j = 0;j < 8;j++)
        {
            if (dot(float4(points[j], 1), perCameraConstants.frustum[i]) <= 0)
                break;
            if (j == 7)
                return true;  //outside
        }
    }

    return false;   //inside
}



//�����Instance��MS�ᵽAS��������û������Per Instance Bounding Box/Sphere
//����AS���ʾ�ɡ�һ�δ���Mesh������meshlet����ɡ�һ�δ���32��meshlet��
//ÿ��AS Thread����һ��Meshlet��AS Groupû�����⺬�塣����MSʱÿ��MS Group����һ��Meshlet
[NumThreads(32, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)  //dtid = groupId * 32 + threadId
{
    bool visible = false;
    //һģһ�����Ƽ�meshlet��ģ�������ر�instanceʱ�Ͳ��ö�shader��
    uint currMeshletCount = dispatchThreadId % meshInfo.MeshletCount;
    uint currInstanceCount = dispatchThreadId / meshInfo.MeshletCount;

    // ������AS Thread��������Ҫ�͸�Mesh��Meshlet����*Instance����һ�£�һһ��Ӧ����
    if (dispatchThreadId < meshInfo.MeshletCount * instanceInfo.InstanceCount)
    {
        // 1. ��׶�޳�
        visible = !IsOutsideFrustum(currMeshletCount, currInstanceCount);

        // 2. �ڵ��޳�
        //visible = true;
    }

    // ͨ�����Ե�Meshlet�Ͱ��Լ����±�push_back��payload�У�push_back����ͨ��WavePrefixCountBits֧�֣�
    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        //һ��һģ���ɡ��˴���ʾAS Group���ȴ���ͬһInstance������Meshlet
        payload.MeshletIndices[index] = currMeshletCount;
        payload.InstanceIndices[index] = currInstanceCount;
    }

    // ����(��group��32����)�ɼ���Meshletsȥ��Mesh Shader
    // ע����������ÿ��AS Group�ڵ��ȶ��MS Group����Dispatch��MS Group�õ���meshlet������ȫ�Ե���
    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, payload);
}

