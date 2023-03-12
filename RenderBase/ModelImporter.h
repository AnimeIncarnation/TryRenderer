#pragma once
#ifndef _MODELIMPORTER_H_
#define _MODELIMPORTER_H_
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <DirectXMath.h>
#include "../stdafx.h"
#include <iostream>
#include <span>
#include "../Resources/Texture.h"
#include "../Resources/UploadBuffer.h"
#include "../Resources/DefaultBuffer.h"

//�洢�����������ع���texture����Ϊpath
//std::unordered_map<const char*, Texture> textureLoaded;

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texCoord;
	bool operator==(const Vertex& vert2)
	{
		if (this->position == vert2.position && this->normal == vert2.normal && this->texCoord == vert2.texCoord)
			return true;
		else
			return false;
	}
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<UINT> indices;
};	

struct Meshlet
{
	UINT VertCount;
	UINT VertOffset;
	UINT PrimitiveCount;
	UINT PrimitiveOffset;
	DirectX::XMFLOAT3 boundingBox[2];	//min, max
	DirectX::XMFLOAT4 boundingSphere;	//pos, r
};

class Mesh2	//for mesh shader
{
public:
	std::vector<Vertex> vertices;
	std::vector<UINT> indices;
	std::vector<UINT> primitives;
	std::vector<Meshlet> meshlets;
	std::unique_ptr<UploadBuffer> meshletCountConstantUpload;
	std::unique_ptr<DefaultBuffer> meshletCountConstantDefault;

	void UploadMeshletCountConstant(DXDevice* dxdevice, ID3D12GraphicsCommandList* cmdList)
	{
		meshletCountConstantUpload = std::make_unique<UploadBuffer>(dxdevice, sizeof(UINT));
		UINT count = meshlets.size();
		meshletCountConstantUpload->CopyData(0, { reinterpret_cast<const byte*>(&count), sizeof(UINT)});

		meshletCountConstantDefault = std::make_unique<DefaultBuffer>(dxdevice, sizeof(UINT));
		cmdList->CopyBufferRegion(meshletCountConstantDefault.get()->GetResource(), 0,
			meshletCountConstantUpload.get()->GetResource(), 0, sizeof(UINT));
	}
	D3D12_GPU_VIRTUAL_ADDRESS GetMeshInfoGPUAddress() const
	{
		return meshletCountConstantDefault->GetGPUAddress();
	}
};



//һ��ģ����Assimp���غ���ܻ�õ��ܶ�Mesh�������˵�ͷ�����壬�ֵȡ������кϲ�����һDraw��������Ĭ�Ϲ��߲�ȡ�İ취
//Ȼ����ModelImporter������Ի�֧��ʹ��Meshlets��������Mesh Shader������������
class ModelImporter
{
	std::vector<Mesh> model;
	std::vector<Mesh2> model2;
	std::string directory;
	Assimp::Importer importer;
	DXDevice* dxDevice;
	ID3D12GraphicsCommandList* cmdList;
	//std::vector<Texture*> textures;

public:
	//pathΪ��SolutionDirΪ��ʼ��ģ���ļ�·��
	void Import(DXDevice* dxdevice, ID3D12GraphicsCommandList* cmdlist, std::string path)
	{
		//���˼����ļ�֮�⣬Assimp���������趨һЩѡ����ǿ�����Ե����������һЩ����ļ���������
		//aiProcess_Triangulate�����ģ�Ͳ��ǣ�ȫ��������������ɣ�����Ҫ��ģ�����е�ͼԪ��״�任Ϊ������
		//aiProcess_FlipUVs����תy�����������
		//aiProcess_GenNormals�����ģ�Ͳ������������Ļ�����Ϊÿ�����㴴������
		//aiProcess_SplitLargeMeshes�����Ƚϴ������ָ�ɸ�С����������������Ⱦ����󶥵������ƣ�ֻ����Ⱦ��С��������ô����ǳ�����
		//aiProcess_OptimizeMeshes�����ϸ�ѡ���෴�����Ὣ���С����ƴ��Ϊһ��������񣬼��ٻ��Ƶ��ôӶ������Ż�
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
		//�ļ���·������ģ��·��ȥ�����һ��б�ܺ�ʣ�µĲ���
		directory = path.substr(0, path.find_last_of('/'));
		dxDevice = dxdevice;
		cmdList = cmdlist;
		ProcessNode( scene->mRootNode, scene);
	}

	void ProcessNode( aiNode* node, const aiScene* scene)
	{
		// ����ڵ����е���������еĻ���
		for (UINT i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			//ProcessModel(dxdevice, i, mesh, scene);
			ProcessModelMeshlet(i, mesh, scene);
		}
		// �������������ӽڵ��ظ���һ����
		for (UINT i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode( node->mChildren[i], scene);
			//std::cout << "��"<<i<<"��Mesh��ɲ�֣�����"
		}
	}

	void ProcessModel( UINT index, aiMesh* mesh, const aiScene* scene)
	{
		Mesh& currMesh = model.emplace_back();
		currMesh.vertices.reserve(mesh->mNumVertices);
		currMesh.indices.reserve(mesh->mNumFaces * 3);
		UINT vertIndex = 0;
		bool ChongFu = false;
		// ���ƶ���λ�á����ߺ���������
		for (UINT i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
			vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
			// �����Ƿ����������ꣿ
			if (mesh->mTextureCoords[0])
				vertex.texCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			else
				vertex.texCoord = { 0.f,0.f };

			//�ж��Ƿ��ظ��������߼����붥��Ͷ�������
			for (UINT iter = 0; iter<currMesh.vertices.size();iter++)
			{
				if (currMesh.vertices[iter] == vertex)
				{
					ChongFu = true;
					currMesh.indices.emplace_back(iter);
					break;
				}
			}
			//�����ظ�������
			if (!ChongFu)
			{
				currMesh.vertices.emplace_back(vertex);
				currMesh.indices.emplace_back(vertIndex++);	   //ʵ����vertIndex����currMesh.vertices.size()-1
			}
			ChongFu = false;
		}
		//// ÿ��mesh��һ������
		//// ��������Assimp���ṩ��������Ĺ��ܣ���Ҫ������stb_image������
		//if (mesh->mMaterialIndex >= 0)
		//{
		//	//aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		//	////ÿ�����ʶ�����ڲ���ÿ���������Ͷ��洢��һ������λ�����顣��ͬ���������Ͷ���aiTextureType_Ϊǰ׺
		//	//loadMaterialTextures(dxdevice, material, aiTextureType_DIFFUSE, "texture_diffuse");
		//	//loadMaterialTextures(dxdevice, material, aiTextureType_SPECULAR, "texture_specular");
		//}
	}

	void ProcessModelMeshlet( UINT index, aiMesh* mesh, const aiScene* scene)
	{
		Mesh2& currMesh = model2.emplace_back();
		currMesh.vertices.reserve(mesh->mNumVertices);
		currMesh.indices.reserve(mesh->mNumFaces * 3);
		UINT vertIndex = 0;
		bool ChongFu = false;
		// ���ƶ���λ�á����ߺ���������
		for (UINT i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
			vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
			// �����Ƿ����������ꣿ
			if (mesh->mTextureCoords[0])
				vertex.texCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			else
				vertex.texCoord = { 0.f,0.f };

			//1. ������������������顣
			//�ж�Vertex�Ƿ��ظ��������߼����붥��Ͷ�������
			for (UINT iter = 0; iter < vertIndex;iter++)
			{
				if (currMesh.vertices[iter] == vertex)
				{
					ChongFu = true;
					currMesh.indices.emplace_back(iter);
					break;
				}
			}
			//�����ظ�������
			if (!ChongFu)
			{
				currMesh.vertices.emplace_back(vertex);
				currMesh.indices.emplace_back(vertIndex++);	   //ʵ����vertIndex����currMesh.vertices.size()-1
			}
			ChongFu = false;
		}

		//{
		//	std::cout << "�����index buffer����Ϊ��"<< currMesh.indices.size() << std::endl;
		//}

		//2. ����Meshlet
		std::vector<UINT> newIndices;
		std::vector<UINT> primitiveIndiceBig;
		UINT indiceIndex = 0;

		//�ȸ���һ��vertex indice��primitive indice������ÿ��primitive indice��һ��UINT��װ)
		for (UINT i = 0;i < currMesh.indices.size(); i++)
		{
			UINT presentIndice = currMesh.indices[i];
			//�ж�Indice�Ƿ��ظ�
			for (UINT iter = 0; iter < indiceIndex;iter++)
			{
				if (newIndices[iter] == presentIndice)
				{
					ChongFu = true;
					primitiveIndiceBig.emplace_back(iter);		//primitiveIndiceBig�з������vertexIndice���±�
					break;
				}
			}
			//�����ظ�������
			if (!ChongFu)
			{
				newIndices.emplace_back(presentIndice);
				primitiveIndiceBig.emplace_back(indiceIndex++);	   //ʵ����vertIndex����currMesh.vertices.size()-1
			}
			ChongFu = false;
		}
		//{
		//	std::cout << "����ǰ��PrimitiveIndiceΪ��" << std::endl;
		//	for (auto x : primitiveIndiceBig)
		//	{
		//		std::cout << x << " ";
		//	}
		//}

		//��ʱvertex indice��ȥ�أ��������vertex indice����������Ҫ�ģ�����Ҫ���������vertexInMeshlet����
		//Ҳ���ɺ���primitive indice�����primitive indiceҲ����������Ҫ�ģ������޸ģ�����������Meshlet
		//vertex indice����64������primitive indice����126*3����ͣ����
		UINT vertexCount = 0;
		UINT vertexOffset = 0;
		UINT primitiveCount = 0;
		UINT primitiveOffset = 0;
		bool nextMeshlet = false;
		UINT NewVertInPrim = 0;
		std::vector<UINT> vertexInMeshlet;	//vertex indices
		UINT originPrimIndic[3];
		//std::cout << std::endl << "���ĺ��PrimitiveIndice��" << std::endl;
		for (UINT i = 0;i < primitiveIndiceBig.size();i+=3)
		{
			nextMeshlet = false;
			NewVertInPrim = 0;
			for (UINT j = 0;j < 3;j++)
			{
				originPrimIndic[j] = primitiveIndiceBig[i + j];
				// Primitive Indice�����ָ��һ���ڸ�Meshlet��û���ֹ���vertex����vertexcount++��������vertexInMeshlet
				for (UINT k = vertexOffset; k < vertexInMeshlet.size();k++)
				{
					//�����vertex�Ѿ����ֹ������һ��
					if (vertexInMeshlet[k] == newIndices[primitiveIndiceBig[i + j]])
					{
						primitiveIndiceBig[i + j] = k;	//
						//std::cout << primitiveIndiceBig[i + j] << " ";
						ChongFu = true;
						break;
					}
				}
				// ��û���ֹ��������vertex��vertexCount��һ
				if (!ChongFu)
				{
					vertexCount++;
					NewVertInPrim++;
					//����������������65�������ˣ���ôȥ���������ͼԪ��ֱ������Meshlet�������¿�ʼ������һ��Meshlet
					if (vertexCount == 65)
					{
						// ����Meshlet
						Meshlet meshlet;
						// ������ͼԪ����һ���¶��㣬VertCount��Ϊ64��������������Ϊ63��������62
						meshlet.VertCount = 65 - NewVertInPrim;
						// ������ͼԪ����һ���¶��㣬vertexInMeshlet���øġ�������ȥ�����һ��Ԫ�أ�������ȥ���������Ԫ��
						vertexInMeshlet.resize(vertexInMeshlet.size() - NewVertInPrim + 1);
						// ͬʱ��primitiveIndiceBig�ж��ڸ�ͼԪ��˵�Ѿ��޸Ĺ�������Ҫ�Ļ�ȥ
						for (UINT k = 0; k <= j;k++)
							primitiveIndiceBig[i + k] = originPrimIndic[k];
						meshlet.VertOffset = vertexOffset;
						meshlet.PrimitiveCount = primitiveCount;
						meshlet.PrimitiveOffset = primitiveOffset;
						currMesh.meshlets.emplace_back(meshlet);

						//{
						//	std::cout << "Meshlet" << currMesh.meshlets.size() - 1 << "����ϢΪ��" << std::endl;
						//	std::cout << "VertCount��" << meshlet.VertCount << "��VertOffset��" << meshlet.VertOffset <<
						//		"��PrimitiveCount��" << meshlet.PrimitiveCount << "��PrimitiveOffset��" << meshlet.PrimitiveOffset << std::endl << std::endl;
						//}
						//if (currMesh.meshlets.size() - 1 == 85)
						//	std::cout << "����ͣ��" << std::endl;
						//if (currMesh.meshlets.size() - 1 == 48)
						//	std::cout << "����ͣ��" << std::endl;
						//����һ��Meshlet
						i -= 3;
						nextMeshlet = true;
						vertexCount = 0;
						primitiveCount = 0;
						primitiveOffset += meshlet.PrimitiveCount;
						vertexOffset += meshlet.VertCount;
						break;
					}
					UINT emplaceInd = newIndices[primitiveIndiceBig[i + j]];
					vertexInMeshlet.emplace_back(emplaceInd);
					primitiveIndiceBig[i + j] = vertexInMeshlet.size() - 1;	//
					//std::cout << primitiveIndiceBig[i + j] << " ";
				}
				ChongFu = false;
			}
			if(!nextMeshlet)
				primitiveCount++;
			if (primitiveCount == 126 ||  i == primitiveIndiceBig.size()-3)
			{
				//����Meshlet
				Meshlet meshlet;
				meshlet.VertCount = vertexCount;
				meshlet.VertOffset = vertexOffset;
				meshlet.PrimitiveCount = primitiveCount;
				meshlet.PrimitiveOffset = primitiveOffset;
				currMesh.meshlets.emplace_back(meshlet);

				//{
				//	std::cout << "Meshlet"<<currMesh.meshlets.size()-1<<"����ϢΪ��"<<std::endl;
				//	std::cout << "VertCount��" << meshlet.VertCount << "��VertOffset��" << meshlet.VertOffset <<
				//		"��PrimitiveCount��" << meshlet.PrimitiveCount << "��PrimitiveOffset��" << meshlet.PrimitiveOffset << std::endl << std::endl;
				//}
				//if (currMesh.meshlets.size() - 1 == 85)
				//	std::cout << "����ͣ��" << std::endl;
				//if (currMesh.meshlets.size() - 1 == 48)
				//	std::cout << "����ͣ��" << std::endl;
				//����һ��Meshlet
				vertexCount = 0;
				primitiveCount = 0;
				primitiveOffset += meshlet.PrimitiveCount;
				vertexOffset += meshlet.VertCount;
			}
		}
		//���桤indice������
		currMesh.indices = vertexInMeshlet;

		//���һ������primitives����uint3��uint���ڴ沼��Ϊ2+10(2)+10(1)+10(0)����Ҫ��ע��primitive��ֵҪ�ȼ�ȥvertex offset
		for (auto& meshlet : currMesh.meshlets)
		{
			//if (meshlet.VertCount < 64)
			//	std::cout << "С��64" << std::endl;
			for (UINT primiCount = 0;primiCount < meshlet.PrimitiveCount;primiCount++)
			{
				primitiveIndiceBig[meshlet.PrimitiveOffset * 3 + primiCount * 3] -= meshlet.VertOffset;
				primitiveIndiceBig[meshlet.PrimitiveOffset * 3 + primiCount * 3 + 1] -= meshlet.VertOffset;
				primitiveIndiceBig[meshlet.PrimitiveOffset * 3 + primiCount * 3 + 2] -= meshlet.VertOffset;
			}
		}
		currMesh.primitives.resize(primitiveIndiceBig.size()/3);
		for (UINT i = 0;i < primitiveIndiceBig.size();i += 3)
		{
			currMesh.primitives[i/3] += primitiveIndiceBig[i];
			currMesh.primitives[i/3] += primitiveIndiceBig[i+1]<<10;
			currMesh.primitives[i/3] += primitiveIndiceBig[i+2]<<20;
		}

		//���һ��VertIndices
		{
			//std::cout << "��Mesh�Ķ�����Ϊ��" << currMesh.vertices.size() << std::endl;
			//std::cout << "���յ�primitiveIndices����Ϊ��" <<currMesh.primitives.size() <<  std::endl; 
			//std::cout << "���յ�VertexIndicesΪ��" << std::endl; 
			//for (UINT ind = 0;ind < currMesh.indices.size();ind++)
			//{
			//	std::cout << currMesh.indices[ind] << " ";
			//}
			//std::cout << std::endl << "finish" << std::endl;
		}

		//�������ѵ�ǰ��Mesh��meshletCount�ϴ���constant buffer
		currMesh.UploadMeshletCountConstant(dxDevice, cmdList);
	}

	void GenerateBoundsMeshlet()
	{
		for (auto& mesh : model2)
		{
			for (auto& meshlet : mesh.meshlets)
			{
				//�������㣬���㶥��������ֵ���õ�AABB
				DirectX::XMFLOAT3 maxFloat{-10000,-10000,-10000}, minFloat{ 10000,10000,10000 };
				//std::cout << "��meshlet�Ķ���Ϊ��" << std::endl;
				for (UINT i = meshlet.VertOffset; i < meshlet.VertOffset + meshlet.VertCount; i++)
				{
					DirectX::XMFLOAT3 pos = mesh.vertices[mesh.indices[i]].position;
					//std::cout << pos.x << " " << pos.y << " " << pos.z << std::endl;
					maxFloat.x = pos.x > maxFloat.x ? pos.x : maxFloat.x;
					maxFloat.y = pos.y > maxFloat.y ? pos.y : maxFloat.y;
					maxFloat.z = pos.z > maxFloat.z ? pos.z : maxFloat.z;

					minFloat.x = pos.x < minFloat.x ? pos.x : minFloat.x;
					minFloat.y = pos.y < minFloat.y ? pos.y : minFloat.y;
					minFloat.z = pos.z < minFloat.z ? pos.z : minFloat.z;
				}
				meshlet.boundingBox[0] = minFloat;
				meshlet.boundingBox[1] = maxFloat;
				//std::cout << "��meshlet�������СֵΪ��"<<std::endl;
				//std::cout << maxFloat.x<< " " << maxFloat.y << " " << maxFloat.z << std::endl;
				//std::cout << minFloat.x<< " " << minFloat.y << " " << minFloat.z << std::endl;

				//����AABB�����BoundingSphere
				DirectX::XMFLOAT3 mid = { (maxFloat.x + minFloat.x) / 2,(maxFloat.y + minFloat.y) / 2,(maxFloat.z + minFloat.z) / 2 };
				DirectX::XMFLOAT3 minToMax = { maxFloat.x - minFloat.x, maxFloat.y - minFloat.y ,maxFloat.z - minFloat.z };
				float r = sqrt(minToMax.x * minToMax.x + minToMax.y * minToMax.y + minToMax.z * minToMax.z)/2;
				meshlet.boundingSphere.x = mid.x;
				meshlet.boundingSphere.y = mid.y;
				meshlet.boundingSphere.z = mid.z;
				meshlet.boundingSphere.w = r;
				//std::cout << "��meshlet�����ĺͰ뾶Ϊ��"<<std::endl;
				//std::cout << mid.x << " " << mid.y << " " << mid.z << " " << r << std::endl;
				//std::cout << std::endl;
			}
		}
	}

	std::span<const Mesh> GetMeshes()const { return model; }
	std::span<const Mesh2> GetMeshesMeshlet()const { return model2; }
};

//	//�����
//	//һ�����ʿ����кܶ�����һ����ȡһ�����͵�����
//	void loadMaterialTextures(DXDevice* dxdevice, aiMaterial* mat, aiTextureType type, std::string typeName)
//	{
//		//������������кܶ��
//		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
//		{
//			aiString str;
//			mat->GetTexture(type, i, &str);
//			// ���������Ѿ����ص�ȫ�����������У�ֱ����
//			auto iter = textureLoaded.find(str.C_Str());
//			if (iter != textureLoaded.end())
//			{
//				textures.emplace_back(&textureLoaded[str.C_Str()]);
//				break;
//			}
//			// �������û�б����أ��ȼ�������
//			// ����Texture����ʹ��stb_image parse����
//			Texture texture = Texture(dxdevice);
//			textureLoaded.insert(str.C_Str(), texture); // ��ӵ��Ѽ��ص�������
//			textures.push_back(loadedTex);
//		}
//	}
//
//	unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma)
//	{
//		std::string filename = std::string(path);
//		filename = directory + '/' + filename;
//
//		int width, height, nrComponents;
//		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
//		if (data)
//		{
//
//			stbi_image_free(data);
//		}
//		else
//		{
//			std::cout << "Texture failed to load at path: " << path << std::endl;
//			stbi_image_free(data);
//		}
//
//		return textureID;
//	}
//	void ClearData() { model.clear(); }
//
//	std::span<Mesh const> GetMeshes()const { return model; }
//};

#endif