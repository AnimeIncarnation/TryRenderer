
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




// 注意Payload是groupshared数据，它仅描述一个SM(Group)内的数据
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

    // 先用BoundingSphere来做一次粗视锥剔除
    float4 center = mul(instanceModelMatrix, float4(currMeshlet.boundingSphere.xyz, 1));
    float radius = currMeshlet.boundingSphere.w;
    for (uint i = 0; i < 6; ++i)
    {
        //当往前走的时，frustum[0]的w竟在后退
        if (dot(center, perCameraConstants.frustum[i]) >= radius) 
        {
            return true;
        }
    }

    //做AABB视锥剔除
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



//必须把Instance由MS提到AS来，否则没法处理Per Instance Bounding Box/Sphere
//启动AS则表示由“一次处理Mesh的所有meshlet”变成“一次处理32个meshlet”
//每个AS Thread处理一个Meshlet，AS Group没有特殊含义。启动MS时每个MS Group处理一个Meshlet
[NumThreads(32, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)  //dtid = groupId * 32 + threadId
{
    bool visible = false;
    //一模一除，推荐meshlet用模，这样关闭instance时就不用动shader了
    uint currMeshletCount = dispatchThreadId % meshInfo.MeshletCount;
    uint currInstanceCount = dispatchThreadId / meshInfo.MeshletCount;

    // 限制总AS Thread的数量，要和该Mesh的Meshlet数量*Instance数量一致，一一对应处理
    if (dispatchThreadId < meshInfo.MeshletCount * instanceInfo.InstanceCount)
    {
        // 1. 视锥剔除
        visible = !IsOutsideFrustum(currMeshletCount, currInstanceCount);

        // 2. 遮挡剔除
        //visible = true;
    }

    // 通过测试的Meshlet就把自己的下标push_back到payload中（push_back语义通过WavePrefixCountBits支持）
    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        //一除一模即可。此处表示AS Group优先处理同一Instance的所有Meshlet
        payload.MeshletIndices[index] = currMeshletCount;
        payload.InstanceIndices[index] = currInstanceCount;
    }

    // 调度(该group的32个中)可见的Meshlets去走Mesh Shader
    // 注意这里是由每个AS Group在调度多个MS Group，被Dispatch的MS Group拿到的meshlet索引完全对的上
    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, payload);
}

