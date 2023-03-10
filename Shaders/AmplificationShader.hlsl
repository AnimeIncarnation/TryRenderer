
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


// ע��Payload��groupshared���ݣ���������һ��SM(Group)�ڵ�����
groupshared Payload payload;
ConstantBuffer<InstanceInfo> instanceInfo   : register(b2);
ConstantBuffer<MeshInfo> meshInfo           : register(b3);


//����AS���ʾ�ɡ�һ�δ���Mesh������meshlet����ɡ�һ�δ���32��meshlet��
//ÿ��AS Thread����һ��Meshlet��AS Groupû�����⺬�塣����MSʱÿ��MS Group����һ��Meshlet
[NumThreads(32, 1, 1)]
void main(uint dispatchThreadId : SV_DispatchThreadID)  //dtid = groupId * 32 + threadId
{
    bool visible = false;

    // ������AS Thread��������Ҫ�͸�Mesh��Meshlet����һ�£�һһ��Ӧ����
    if (dispatchThreadId < meshInfo.MeshletCount)
    {
        // ��׶����
        //visible = IsVisible(MeshletCullData[dispatchThreadId], Instance.World, Instance.Scale, Constants.CullViewPosition);
        visible = true;
    }

    // ͨ�����Ե�Meshlet�Ͱ��Լ����±�push_back��payload�У�push_back����ͨ��WavePrefixCountBits֧�֣�
    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        payload.MeshletIndices[index] = dispatchThreadId;
    }

    // ����(��group��32����)�ɼ���Meshletsȥ��Mesh Shader
    // ע����������ÿ��AS Group�ڵ��ȶ��MS Group����Dispatch��MS Group�õ���meshlet������ȫ�Ե���
    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount * instanceInfo.InstanceCount, 1, 1, payload);
}

