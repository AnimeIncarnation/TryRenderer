
//ASÿ֡��ɢٷ�Cluster���ڵ��Դ����׶�޳����۵��Դ��ϸ��׶����(Cluster����)
//������clusters��ע��points��ÿ���̴߳���һ��cluster
//�ڰ��±�ע��culledPointLightIndices��ÿ���̴߳���һ�����Դ�����Ե��Դ����������С��max(meshletCount * instanceCount, clusterNum)
//�۰�lightStart��lightCountע��clusters��Ȼ������lightAssignBuffer��ÿ���̴߳���һ��cluster���о����У���Ϊ����������Ҫ�Ⱥ�˳��ģ�
//������Ҫ������֮�����԰����ŵ�MeshShader����
struct PointLight   //���Դ�İ�Χ��
{
    float4 data;    //float3��position��float��radius
};
StructuredBuffer<PointLight> allPointLights;
RWStructuredBuffer<uint> culledPointLightIndices;
struct Cluster
{
    float3 points[8]; //��ԶȻ������Ͽ�ʼ˳ʱ��
    uint lightStart;
    uint lightCount;
};
RWStructuredBuffer<Cluster> clusters;
RWStructuredBuffer<uint> lightAssignBuffer; //�洢����culledPointLightIndices���±꣬�൱�ڶ���ָ��