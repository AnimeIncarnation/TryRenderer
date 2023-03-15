
//AS每帧完成①分Cluster，②点光源的视锥剔除，③点光源的细视锥分配(Cluster分配)
//①先在clusters中注入points。每个线程处理一个cluster
//②把下标注入culledPointLightIndices。每个线程处理一个点光源，所以点光源的数量必须小于max(meshletCount * instanceCount, clusterNum)
//③把lightStart和lightCount注入clusters，然后生成lightAssignBuffer。每个线程处理一个cluster（感觉不行，因为这三部是需要先后顺序的，
//三必须要在两者之后，所以把三放到MeshShader？）
struct PointLight   //点光源的包围盒
{
    float4 data;    //float3的position和float的radius
};
StructuredBuffer<PointLight> allPointLights;
RWStructuredBuffer<uint> culledPointLightIndices;
struct Cluster
{
    float3 points[8]; //近远然后从左上开始顺时针
    uint lightStart;
    uint lightCount;
};
RWStructuredBuffer<Cluster> clusters;
RWStructuredBuffer<uint> lightAssignBuffer; //存储的是culledPointLightIndices的下标，相当于二级指针