//#define ROOT_SIG "CBV(b0), \
//                  CBV(b1)"
//
struct PerCameraConstants
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 vpMatrix;
};

struct PerLightConstant
{
    float3 stength;
    float fallOffStart;
    float3 position;
    float fallOffEnd;
    float3 direction;
    float spotPower;
};

struct VertexOut
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
};

struct Vertex
{
    float4 position;
    float3 normal;
    float2 texCoord;
};

struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimitiveCount;
    uint PrimitiveOffset;
};

struct PerInstanceData
{
    float4x4 modelMatrix;
};

struct DrawParams
{
    uint InstanceCount;
    uint InstanceOffset;
};

ConstantBuffer<PerCameraConstants> perCameraConstants : register(b0);
ConstantBuffer<PerLightConstant> perLightConstants: register(b1);
ConstantBuffer<DrawParams> drawParams          : register(b2);
RWStructuredBuffer<Meshlet> Meshlets          : register(u0);
RWStructuredBuffer<Vertex>  Vertices          : register(u1);
RWStructuredBuffer<uint>    VertexIndices     : register(u2);
RWStructuredBuffer<uint>    PrimitiveIndices  : register(u3);
StructuredBuffer<PerInstanceData>  InstanceData : register(t0);


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
[NumThreads(128, 1, 1)]
void main(
    uint groupThreadId : SV_GroupThreadID,
    uint groupId : SV_GroupID,
    out vertices VertexOut outVerts[64],
    out indices uint3 outIndices[126]
)
{
    //Goup数等于InstanceCount * Meshlet的数量
    uint meshletIndex = groupId / drawParams.InstanceCount;
    uint startInstance = groupId % drawParams.InstanceCount;
    uint instanceCount = 1;


    Meshlet m = Meshlets[meshletIndex];
    uint vertCount = m.VertCount * instanceCount;
    uint primCount = m.PrimitiveCount * instanceCount;
    SetMeshOutputCounts(vertCount, primCount);
    

    if (groupThreadId < m.VertCount)
    {
        uint vertexIndex = VertexIndices[m.VertOffset + groupThreadId];
            
        Vertex vin = Vertices[vertexIndex];
        VertexOut vout;
        vout.position =  mul(InstanceData[startInstance].modelMatrix, vin.position);
        vout.position = mul(perCameraConstants.vpMatrix, vout.position);
        vout.normal = vin.normal;
        vout.color = float4(0,0,0,1);
        outVerts[groupThreadId] = vout;
    }
    
    if (groupThreadId < m.PrimitiveCount)
    {
        uint packedIndices = PrimitiveIndices[m.PrimitiveOffset + groupThreadId];
        outIndices[groupThreadId] = uint3(packedIndices & 0x3FF, (packedIndices >> 10) & 0x3FF, (packedIndices >> 20) & 0x3FF);
    }
}