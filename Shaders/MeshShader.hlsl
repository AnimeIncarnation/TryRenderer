//#define ROOT_SIG "CBV(b0), \
//                  CBV(b1)"
//
//struct PerCameraConstants
//{
//    float4x4 viewMatrix;
//    float4x4 projMatrix;
//    float4x4 vpMatrix;
//};
//
//struct PerLightConstant
//{
//    float3 stength;
//    float fallOffStart;
//    float3 position;
//    float fallOffEnd;
//    float3 direction;
//    float spotPower;
//};

//struct MeshInfo
//{
//    uint IndexBytes;
//    uint MeshletOffset;
//};
//
//struct Vertex
//{
//    float3 Position;
//    float3 Normal;
//};

struct VertexOut
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
    //float3 PositionVS   : POSITION0;
    //uint   MeshletIndex : COLOR0;
};
//
//ConstantBuffer<PerCameraConstants> perCameraConstants : register(b0);
//ConstantBuffer<PerLightConstant> perLightConstants: register(b1);
//struct Meshlet
//{
//    uint VertCount;
//    uint VertOffset;
//    uint PrimitiveCount;
//    uint PrimitiveOffset;
//};

//ConstantBuffer<MeshInfo>  MeshInfo            : register(b2);
//
//StructuredBuffer<Vertex>  Vertices            : register(t0);
//StructuredBuffer<Meshlet> Meshlets            : register(t1);
//ByteAddressBuffer         UniqueVertexIndices : register(t2);
//StructuredBuffer<uint>    PrimitiveIndices    : register(t3);


/////
// Data Loaders

//uint3 UnpackPrimitive(uint primitive)
//{
//    // Unpacks a 10 bits per index triangle from a 32-bit uint.
//    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
//}
//
//uint3 GetPrimitive(Meshlet m, uint index)
//{
//    return UnpackPrimitive(PrimitiveIndices[m.PrimOffset + index]);
//}
//
//uint GetVertexIndex(Meshlet m, uint localIndex)
//{
//    localIndex = m.VertOffset + localIndex;
//
//    if (MeshInfo.IndexBytes == 4) // 32-bit Vertex Indices
//    {
//        return UniqueVertexIndices.Load(localIndex * 4);
//    }
//    else // 16-bit Vertex Indices
//    {
//        // Byte address must be 4-byte aligned.
//        uint wordOffset = (localIndex & 0x1);
//        uint byteOffset = (localIndex / 2) * 4;
//
//        // Grab the pair of 16-bit indices, shift & mask off proper 16-bits.
//        uint indexPair = UniqueVertexIndices.Load(byteOffset);
//        uint index = (indexPair >> (wordOffset * 16)) & 0xffff;
//
//        return index;
//    }
//}
//
//VertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex)
//{
//    Vertex v = Vertices[vertexIndex];
//
//    VertexOut vout;
//    vout.PositionVS = mul(float4(v.Position, 1), Globals.WorldView).xyz;
//    vout.PositionHS = mul(float4(v.Position, 1), Globals.WorldViewProj);
//    vout.Normal = mul(float4(v.Normal, 0), Globals.World).xyz;
//    vout.MeshletIndex = meshletIndex;
//
//    return vout;
//}


//[RootSignature(ROOT_SIG)]
[OutputTopology("triangle")]
[NumThreads(1, 1, 1)]
void main(
    //uint gtid : SV_GroupThreadID,
    //uint gid : SV_GroupID,
    out vertices VertexOut outVerts[3],
    out indices uint3 outIndices[1]
)
{
    SetMeshOutputCounts(3, 1);
    outVerts[0].position = float4(0.5, 0.5, -0.5, 1);
    outVerts[0].normal = float3(0, 0, 0);
    outVerts[0].color = float4(0.5,0.5,0.5,0);

    outVerts[1].position = float4(0.5, -0.5, 0.5, 1);
    outVerts[1].normal = float3(0, 0, 0);
    outVerts[1].color = float4(0, 0, 0, 0);

    outVerts[2].position = float4(-0.5, 0.25, -0.25, 1);
    outVerts[2].normal = float3(0, 0, 0);
    outVerts[2].color = float4(0.5, 0.5, 0.5, 0);

    outIndices[0] = uint3(0, 1, 2);
    //Meshlet m = Meshlets[MeshInfo.MeshletOffset + gid];
    //if (gtid < m.PrimCount)
    //{
    //    outIndices[gtid] = GetPrimitive(m, gtid);
    //}
    //if (gtid < m.VertCount)
    //{
    //    uint vertexIndex = GetVertexIndex(m, gtid);
    //    verts[gtid] = GetVertexAttributes(gid, vertexIndex);
    //}

    //for (int i = 0;i < 8;i++)
    //{
    //    outVertes[i].pos = float4(0, 0, 0, 0);
    //    outVertes[i].normal = float3(0, 0, 0);
    //    outVertes[i].color = float4(0, 0, 0, 0);
    //}
    //for (int i = 0;i < 12;i++)
    //{
    //    outIndices[i] = uint3(0, 1, 2);
    //}

}