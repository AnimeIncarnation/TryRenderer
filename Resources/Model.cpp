#include "Model.h"
#include "../DXMath/MathHelper.h"
#include "../DXSampleHelper.h"
#include <string>


//struct Meshlet
//{
//    UINT VertCount;
//    UINT VertOffset;
//    UINT PrimitiveCount;
//    UINT PrimitiveOffset;
//};

Model::Model(DXDevice* device):dxdevice(device) {}

void Model::SetInputLayout(const rtti::ElementStruct* vbStruct, UINT slot, const std::string& name)
{
    vertexStruct = vbStruct;
    layout.first = name;
	vertexStruct->GetMeshLayout(slot, layout.second);
    vertexStruct->InsertGlobalLayout(name, layout.second);
}

void Model::ParseFromAssimpToMeshletAndUpload(ID3D12GraphicsCommandList* cmdList, ModelImporter* modelImporter)
{
    if (parsed) 
        return;
    parsed = true;

    //��Assimp�õ���ÿ��mesh����
    //ÿ��mesh�洢һ��defaultbuffer
    UINT meshCount = modelImporter->GetMeshesMeshlet().size();
    //std::cout << "��Model" << "����" << meshCount << "��mesh" << std::endl;
    //std::cout << "���������������������ÿ��Mesh����Ϣ��������" << std::endl;
    for (UINT i = 0; i < meshCount;i++)
    {
        //1. ��ȡ��Mesh�Ķ�����������������
        const Mesh2& presentMesh = modelImporter->GetMeshesMeshlet()[i];
        UINT64 vCount = presentMesh.vertices.size();
        UINT64 iCount = presentMesh.indices.size();
        UINT64 pCount = presentMesh.primitives.size();
        UINT64 mCount = presentMesh.meshlets.size();
        vertexCount.emplace_back(vCount);
        indiceCount.emplace_back(iCount);
        meshletCount.emplace_back(mCount);

        //2. ����CPU�˵�VertexBuffer������������Ϊ����Ҫ��position��3ά�ĳ�4ά����Ҫparseһ�¡������Ĳ���ֱ�Ӵ�����
        std::vector<byte> modelVertices(vertexStruct->structSize * vCount);
        byte* vertexBytePtr = modelVertices.data();
        for (UINT j = 0;j < vCount;j++)
        {
            Vertex presentVertex = presentMesh.vertices[j];
            //Parse�㷨
            DirectX::XMFLOAT4 position = { presentVertex.position, 1 };
            DirectX::XMFLOAT3 normal = presentVertex.normal;
            DirectX::XMFLOAT2 texCoord = presentVertex.texCoord;
            rtti::Vertex2::Instance().position.Get(vertexBytePtr) = position;
            rtti::Vertex2::Instance().normal.Get(vertexBytePtr) = normal;
            rtti::Vertex2::Instance().texCoord.Get(vertexBytePtr) = texCoord;
            vertexBytePtr += rtti::Vertex2::Instance().structSize;
            {
              //output
              //std::cout << "���Mesh" << i << "�Ķ��㣺" << std::endl;
              //std::cout << position.x << " " << position.y << " " << position.z << std::endl;
              //Math::Matrix4 vpMatrix = mainCamera->GetProjectionMatrix() * mainCamera->GetViewMatrix();
              //Math::XMFLOAT4 p = vpMatrix * position;
              //std::cout << p.x << " " << p.y << " " << p.z << " " << p.w << std::endl;
            }
        }

        //�����Ϣ
        //{
        //    std::cout << "Mesh" << i+1 <<"��"<< presentMesh.meshlets.size() << "��meshlet" << std::endl;
        //    std::cout << "���Mesh" << i+1 << "�Ķ���������" << std::endl;
        //    for (UINT j = 0;j < iCount;j++)
        //    {
        //        std::cout << presentMesh.indices[j] << " ";
        //    }
        //    std::cout << std::endl;
        //    std::cout << "���Mesh" << i+1 << "��ͼԪ������" << std::endl;
        //    for (UINT j = 0;j < pCount;j++)
        //    {
        //        std::cout << "(" <<(UINT)(presentMesh.primitives[j] << 22 >> 22) << "," << (UINT)(presentMesh.primitives[j] << 12 >> 22) << "," << (UINT)(presentMesh.primitives[j] >> 20) <<")��";
        //    }
        //    std::cout << std::endl;
        //    for (UINT k = 0; k < presentMesh.meshlets.size();k++)
        //    {
        //        std::cout << "Mesh" << i+1 << "�ĵ�" << k+1 << "��Meshlet����ϢΪ��" << std::endl;
        //        std::cout << "VertexCount��" << presentMesh.meshlets[k].VertCount << "��"
        //                  << "VertexOffset��" << presentMesh.meshlets[k].VertOffset << "��"
        //                  << "PrimitiveCount��" << presentMesh.meshlets[k].PrimitiveCount << "��"
        //                  << "PrimitiveOffset��" << presentMesh.meshlets[k].PrimitiveOffset << std::endl;
        //    }
        //    std::cout << std::endl << std::endl;
        //}

        //3. ����UploadBuffer����CPUBuffer�ϴ���UploadBuffer
        uploadBufferVertex.emplace_back(dxdevice, modelVertices.size());
        vertexIndicePerMeshUpload.emplace_back(dxdevice, iCount * sizeof(UINT));
        primitiveIndicePerMeshUpload.emplace_back(dxdevice, pCount * sizeof(UINT));
        meshletsPerMeshUpload.emplace_back(dxdevice, mCount * sizeof(Meshlet));
        
        uploadBufferVertex[i].CopyData(0, modelVertices);  //offsetΪ0�����ݴ�cubeVertices�п���
        vertexIndicePerMeshUpload[i].CopyData(0, { reinterpret_cast<byte const*>(presentMesh.indices.data()), iCount * sizeof(UINT) });
        primitiveIndicePerMeshUpload[i].CopyData(0, { reinterpret_cast<byte const*>(presentMesh.primitives.data()), pCount * sizeof(UINT) });
        meshletsPerMeshUpload[i].CopyData(0, { reinterpret_cast<byte const*>(presentMesh.meshlets.data()), mCount * sizeof(Meshlet) });



        //4. ��UploadBuffer����ת����DefaultBuffer
        ID3D12Resource* vertexDefault = vertexPerMesh.emplace_back(dxdevice, modelVertices.size()).GetResource();
        ID3D12Resource* indexDefault = vertexIndicePerMesh.emplace_back(dxdevice, iCount * sizeof(UINT)).GetResource();
        ID3D12Resource* primitiveDefault = primitiveIndicePerMesh.emplace_back(dxdevice, pCount * sizeof(UINT)).GetResource();
        ID3D12Resource* meshletDefault = meshletsPerMesh.emplace_back(dxdevice, mCount * sizeof(Meshlet)).GetResource();
        cmdList->CopyBufferRegion(vertexDefault, 0, uploadBufferVertex[i].GetResource(), 0, uploadBufferVertex[i].GetSize());
        cmdList->CopyBufferRegion(indexDefault, 0, vertexIndicePerMeshUpload[i].GetResource(), 0, vertexIndicePerMeshUpload[i].GetSize());
        cmdList->CopyBufferRegion(primitiveDefault, 0, primitiveIndicePerMeshUpload[i].GetResource(), 0, primitiveIndicePerMeshUpload[i].GetSize());
        cmdList->CopyBufferRegion(meshletDefault, 0, meshletsPerMeshUpload[i].GetResource(), 0, meshletsPerMeshUpload[i].GetSize());
    
        //5. ���MeshInfo Address
        meshInfoAddress.emplace_back(presentMesh.GetMeshInfoGPUAddress());
    }
    //6. ���ô���UAV��ͼ����Ϊ����ֱ�Ӱ���Щ��Դ�ľ��Ե�ַ������view����Ӵ���
    
}


void Model::ParseFromAssimpAndUpload(ID3D12GraphicsCommandList* cmdList, ModelImporter* modelImporter)
{
    if (parsed)
        return;
    parsed = true;

    //��Assimp�õ���ÿ��mesh����
    //ÿ��mesh�洢һ��defaultbuffer
    UINT meshCount = modelImporter->GetMeshes().size();
    for (UINT i = 0; i < meshCount;i++)
    {
        //1. ��ȡ��Mesh�Ķ�����������������
        const Mesh& presentMesh = modelImporter->GetMeshes()[i];
        UINT64 vCount = presentMesh.vertices.size();
        UINT64 iCount = presentMesh.indices.size();
        vertexCount.emplace_back(vCount);
        indiceCount.emplace_back(iCount);

        //2. ����CPU�˵�VertexBuffer������������Ϊ�������Լ���ShaderVertexInput����������Ҫparseһ�¡���indice buffer����parseֱ���ü���
        //CPU�˵�vertexbuffer�����д洢�����꼴��
        std::vector<byte> modelVertices(vertexStruct->structSize * vCount);
        byte* vertexBytePtr = modelVertices.data();
        //std::cout << "���Mesh" << i << "�Ķ��㣺" << std::endl;
        for (UINT j = 0;j < vCount;j++)
        {
            Vertex presentVertex = presentMesh.vertices[j];
            //Parse�㷨
            DirectX::XMFLOAT4 position = { presentVertex.position, 1 };
            DirectX::XMFLOAT3 normal = presentVertex.normal;
            DirectX::XMFLOAT2 texCoord = presentVertex.texCoord;
            rtti::Vertex2::Instance().position.Get(vertexBytePtr) = position;
            rtti::Vertex2::Instance().normal.Get(vertexBytePtr) = normal;
            rtti::Vertex2::Instance().texCoord.Get(vertexBytePtr) = texCoord;
            vertexBytePtr += rtti::Vertex::Instance().structSize;
            {
                //output
                //std::cout << position.x << " " << position.y << " " << position.z << std::endl;
                //Math::Matrix4 vpMatrix = mainCamera->GetProjectionMatrix() * mainCamera->GetViewMatrix();
                //Math::XMFLOAT4 p = vpMatrix * position;
                //std::cout << p.x << " " << p.y << " " << p.z << " " << p.w << std::endl;
            }
        }
        //std::cout << "�ܶ�����Ϊ��"<<presentMesh.vertices.size() << std::endl;
        //std::cout << "���Mesh" << i << "��������" << std::endl;
        //for (UINT j = 0;j < iCount;j++)
        //{
        //    std::cout << presentMesh.indices[j] << " ";
        //}
        //std::cout << "��������Ϊ��"<<presentMesh.indices.size() << std::endl;



        //3. ��CPUBuffer�ϴ���UploadBuffer
        uploadBufferVertex.emplace_back(dxdevice, modelVertices.size());
        uploadBufferIndex.emplace_back(dxdevice, iCount * sizeof(UINT));
        uploadBufferVertex[i].CopyData(0, modelVertices);  //offsetΪ0�����ݴ�cubeVertices�п���
        uploadBufferIndex[i].CopyData(0, { reinterpret_cast<byte const*>(presentMesh.indices.data()), iCount * sizeof(UINT) });
        std::cout << i << std::endl;


        //4. ��UploadBuffer����ת����DefaultBuffer
        ID3D12Resource* vertexDefault = vertexPerMesh.emplace_back(dxdevice, modelVertices.size()).GetResource();
        ID3D12Resource* indexDefault = indexPerMesh.emplace_back(dxdevice, iCount * sizeof(UINT)).GetResource();
        cmdList->CopyBufferRegion(vertexDefault, 0, uploadBufferVertex[i].GetResource(), 0, uploadBufferVertex[i].GetSize());
        cmdList->CopyBufferRegion(indexDefault, 0, uploadBufferIndex[i].GetResource(), 0, uploadBufferIndex[i].GetSize());
    }

    //5. �����������������������ͼ
    for (size_t i = 0; i < vertexPerMesh.size(); ++i)
    {
        vertexBufferView.emplace_back();
        //��DefaultBuffer�õ�VertexBufferView��View/Span������ʼ��ַ��size��stride
        vertexBufferView[i].BufferLocation = vertexPerMesh[i].GetGPUAddress();
        vertexBufferView[i].SizeInBytes = vertexPerMesh[i].GetSize();
        vertexBufferView[i].StrideInBytes = vertexBufferView[i].SizeInBytes / vertexCount[i];
    }
    for (size_t i = 0; i < indexPerMesh.size(); ++i)
    {
        indexBufferView.emplace_back();
        indexBufferView[i].BufferLocation = indexPerMesh[i].GetGPUAddress();
        indexBufferView[i].SizeInBytes = indexPerMesh[i].GetSize();
        indexBufferView[i].Format = DXGI_FORMAT_R32_UINT;
    }
}



//����Ǵ�spanֱ�Ӵ�������ô���洢�ڵ�һ��Buffer����
void Model::ParseFromSpansAndUpload(ID3D12GraphicsCommandList* cmdList, std::span<DirectX::XMFLOAT3 const> positions, std::span<UINT const> indices)
{
    if (parsed) 
        return;
    parsed = true;

    UINT vCount = positions.size();
    UINT iCount = indices.size();
    vertexCount.emplace_back(vCount);
    indiceCount.emplace_back(iCount);


    //1. CPU��Buffer
    std::vector<byte> modelVertices(rtti::Vertex::Instance().structSize * vCount);
    byte* vertexBytePtr = modelVertices.data();
    for (int i = 0;i < vCount;i++)
    {

        DirectX::XMFLOAT4 position = { positions[i], 1 };
        rtti::Vertex::Instance().position.Get(vertexBytePtr) = position;
        DirectX::XMFLOAT3 normal = { 0,0,0 };
        rtti::Vertex::Instance().normal.Get(vertexBytePtr) = normal;
        DirectX::XMFLOAT4 color(position.x + 0.5f, position.y + 0.5f, position.z + 0.5f, 1);
        rtti::Vertex::Instance().color.Get(vertexBytePtr) = color;
        vertexBytePtr += rtti::Vertex::Instance().structSize;
        //output
        //std::cout << position.x << " " << position.y << " " << position.z << std::endl;
    }
    //2. ����UploadBuffer�������Ͷ������ݵ�UploadBuffer
    uploadBufferVertex.emplace_back(dxdevice, modelVertices.size());
    uploadBufferIndex.emplace_back(dxdevice, iCount * sizeof(UINT));
    uploadBufferVertex[0].CopyData(0, modelVertices);
    uploadBufferIndex[0].CopyData(0, { reinterpret_cast<byte const*>(indices.data()), iCount * sizeof(UINT) });

    //4. ��UploadBuffer����ת����DefaultBuffer
    vertexPerMesh.push_back({ dxdevice, modelVertices.size() });
    indexPerMesh.push_back({ dxdevice, iCount * sizeof(UINT) });
    cmdList->CopyBufferRegion(vertexPerMesh[0].GetResource(), 0, uploadBufferVertex[0].GetResource(), 0, uploadBufferVertex[0].GetSize());
    cmdList->CopyBufferRegion(indexPerMesh[0].GetResource(), 0, uploadBufferIndex[0].GetResource(), 0, uploadBufferIndex[0].GetSize());


    //5. �����������������������ͼ
    vertexBufferView.push_back(D3D12_VERTEX_BUFFER_VIEW{});
    //��DefaultBuffer�õ�VertexBufferView��View/Span������ʼ��ַ��size��stride
    vertexBufferView[0].BufferLocation = vertexPerMesh[0].GetGPUAddress();
    vertexBufferView[0].SizeInBytes = vertexPerMesh[0].GetSize();
    vertexBufferView[0].StrideInBytes = vertexBufferView[0].SizeInBytes / vertexCount[0];
    indexBufferView.push_back(D3D12_INDEX_BUFFER_VIEW{});
    indexBufferView[0].BufferLocation = indexPerMesh[0].GetGPUAddress();
    indexBufferView[0].SizeInBytes = indexPerMesh[0].GetSize();
    indexBufferView[0].Format = DXGI_FORMAT_R32_UINT;
}


const std::vector<D3D12_VERTEX_BUFFER_VIEW>& Model::GetVertexBufferView()const
{
    return vertexBufferView;
}

const std::vector<D3D12_INDEX_BUFFER_VIEW>& Model::GetIndexBufferView()const
{
    return indexBufferView;
}

