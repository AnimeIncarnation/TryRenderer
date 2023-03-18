struct PerCameraConstants
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float4x4 vpMatrix;
    float4x4 pInv;
    float4x4 vpInv;
    float4 frustum[6];
};

struct SceneConstant
{
    float3 parallelLightDirection;
    uint pointLightCount;
    uint maxLightCountPerCluster;
    float3 parallelLightStrength;
    float nearZ;
    float farZ;

};


/*

1. Raster Shader Signature
ConstantBuffer<PerCameraConstants>  perCameraConstants  : register(b0);
ConstantBuffer<SceneConstant> SceneConstant             : register(b1);
ConstantBuffer<InstanceInfo>        instanceInfo        : register(b2);
ConstantBuffer<MeshInfo>            meshInfo            : register(b3);

RWStructuredBuffer<Meshlet>         Meshlets            : register(u0);
RWStructuredBuffer<Vertex>  Vertices                    : register(u1);
RWStructuredBuffer<uint>    VertexIndices               : register(u2);
RWStructuredBuffer<uint>    PrimitiveIndices            : register(u3);
RWStructuredBuffer<Cluster> Clusters                    : register(u4);
RWStructuredBuffer<uint> CulledPointLightIndices        : register(u5);
RWStructuredBuffer<uint> LightAssignBuffer              : register(u6);

StructuredBuffer<PerInstanceData>   InstanceData        : register(t0);
StructuredBuffer<PointLight> AllPointLights             : register(t1);


2. Compute Shader Signature
ConstantBuffer<PerCameraConstants>  perCameraConstants  : register(b0);
ConstantBuffer<SceneConstant> SceneConstant             : register(b1);
RWStructuredBuffer<Cluster> Clusters                    : register(u0);
RWStructuredBuffer<uint> CulledPointLightIndices        : register(u1);
RWStructuredBuffer<uint> LightAssignBuffer              : register(u2);
StructuredBuffer<PointLight> AllPointLights             : register(t0);
*/