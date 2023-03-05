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

//�洢�����������ع���texture����Ϊpath
//std::unordered_map<const char*, Texture> textureLoaded;

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texCoord;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<UINT> indices;
};

struct Meshlet
{
	UINT vertices[64];
	UINT indices[126];
	UINT indexCount;
	UINT vertexCount;
};

//һ��ģ����Assimp���غ���ܻ�õ��ܶ�Mesh�������˵�ͷ�����壬�ֵȡ������кϲ�����һDraw��������Ĭ�Ϲ��߲�ȡ�İ취
//Ȼ����ModelImporter������Ի�֧��ʹ��Meshlets��������Mesh Shader������������
class ModelImporter
{
	std::vector<Mesh> model;
	std::vector<Meshlet> meshlets;
	std::string directory;
	Assimp::Importer importer;
	DXDevice* dxDevice;
	//std::vector<Texture*> textures;

public:
	//pathΪ��SolutionDirΪ��ʼ��ģ���ļ�·��
	void Import(DXDevice* dxdevice, std::string path)
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
		ProcessNode(dxdevice, scene->mRootNode, scene);
	}

	void ProcessNode(DXDevice* dxdevice, aiNode* node, const aiScene* scene)
	{
		// ����ڵ����е���������еĻ���
		for (UINT i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessModel(dxdevice, i, mesh, scene);
		}
		// �������������ӽڵ��ظ���һ����
		for (UINT i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(dxdevice, node->mChildren[i], scene);
		}
	}

	void ProcessModel(DXDevice* dxdevice, UINT index, aiMesh* mesh, const aiScene* scene)
	{
		Mesh& currMesh = model.emplace_back();
		currMesh.vertices.reserve(mesh->mNumVertices);
		currMesh.indices.reserve(mesh->mNumFaces * 3);

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
			currMesh.vertices.emplace_back(vertex);
		}

		// ��������
		for (UINT i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				currMesh.indices.emplace_back(face.mIndices[j]);
		}

		// ÿ��mesh��һ������
		// ��������Assimp���ṩ��������Ĺ��ܣ���Ҫ������stb_image������
		if (mesh->mMaterialIndex >= 0)
		{
			//aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			////ÿ�����ʶ�����ڲ���ÿ���������Ͷ��洢��һ������λ�����顣��ͬ���������Ͷ���aiTextureType_Ϊǰ׺
			//loadMaterialTextures(dxdevice, material, aiTextureType_DIFFUSE, "texture_diffuse");
			//loadMaterialTextures(dxdevice, material, aiTextureType_SPECULAR, "texture_specular");
		}
	}

	std::span<const Mesh> GetMeshes()const { return model; }
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