
struct Payload
{
    uint MeshletIndices[32];
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

//bool IsVisible(CullData c, float4x4 world, float scale, float3 viewPos)
//{
//    if ((Instance.Flags & CULL_FLAG) == 0)
//        return true;
//
//    // Do a cull test of the bounding sphere against the view frustum planes.
//    float4 center = mul(float4(c.BoundingSphere.xyz, 1), world);
//    float radius = c.BoundingSphere.w * scale;
//
//    for (int i = 0; i < 6; ++i)
//    {
//        if (dot(center, Constants.Planes[i]) < -radius)
//        {
//            return false;
//        }
//    }
//
//    // Do normal cone culling
//    if (IsConeDegenerate(c))
//        return true; // Cone is degenerate - spread is wider than a hemisphere.
//
//    // Unpack the normal cone from its 8-bit uint compression
//    float4 normalCone = UnpackCone(c.NormalCone);
//
//    // Transform axis to world space
//    float3 axis = normalize(mul(float4(normalCone.xyz, 0), world)).xyz;
//
//    // Offset the normal cone axis from the meshlet center-point - make sure to account for world scaling
//    float3 apex = center.xyz - axis * c.ApexOffset * scale;
//    float3 view = normalize(viewPos - apex);
//
//    // The normal cone w-component stores -cos(angle + 90 deg)
//    // This is the min dot product along the inverted axis from which all the meshlet's triangles are backface
//    if (dot(view, -axis) > normalCone.w)
//    {
//        return false;
//    }
//
//    // All tests passed - it will merit pixels
//    return true;
//}
//


// 注意Payload是groupshared数据，它仅描述一个SM(Group)内的数据
groupshared Payload payload;
ConstantBuffer<InstanceInfo> instanceInfo   : register(b2);
ConstantBuffer<MeshInfo> meshInfo           : register(b3);


//启动AS则表示由“一次处理Mesh的所有meshlet”变成“一次处理32个meshlet”
//每个AS Thread处理一个Meshlet，AS Group没有特殊含义。启动MS时每个MS Group处理一个Meshlet
[NumThreads(32, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)  //dtid = groupId * 32 + threadId
{
    bool visible = false;

    // 限制总AS Thread的数量，要和该Mesh的Meshlet数量一致，一一对应处理
    if (dispatchThreadId < meshInfo.MeshletCount)
    {
        // 视锥测试
        //visible = IsVisible(MeshletCullData[dispatchThreadId], Instance.World, Instance.Scale, Constants.CullViewPosition);
        visible = true;
    }

    // 通过测试的Meshlet就把自己的下标push_back到payload中（push_back语义通过WavePrefixCountBits支持）
    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        payload.MeshletIndices[index] = dispatchThreadId;
    }

    // 调度(该group的32个中)可见的Meshlets去走Mesh Shader
    // 注意这里是由每个AS Group在调度多个MS Group，被Dispatch的MS Group拿到的meshlet索引完全对的上
    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount * instanceInfo.InstanceCount, 1, 1, payload);
}

