#pragma once
#ifndef _MODEL_H_
#define _MODEL_H_
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <DirectXMath.h>
#include "../stdafx.h"
#include <iostream>
#include <span>

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texCoord;
};

struct Model
{
	std::vector<Vertex> vertices;
	std::vector<UINT> indices;
};

class ModelImporter
{
	std::vector<Model> models;
	Assimp::Importer importer;

public:
	void Import(std::string path)
	{
		//���˼����ļ�֮�⣬Assimp���������趨һЩѡ����ǿ�����Ե����������һЩ����ļ���������
		//aiProcess_Triangulate�����ģ�Ͳ��ǣ�ȫ��������������ɣ�����Ҫ��ģ�����е�ͼԪ��״�任Ϊ������
		//aiProcess_FlipUVs����תy�����������
		//aiProcess_GenNormals�����ģ�Ͳ������������Ļ�����Ϊÿ�����㴴������
		//aiProcess_SplitLargeMeshes�����Ƚϴ������ָ�ɸ�С����������������Ⱦ����󶥵������ƣ�ֻ����Ⱦ��С��������ô����ǳ�����
		//aiProcess_OptimizeMeshes�����ϸ�ѡ���෴�����Ὣ���С����ƴ��Ϊһ��������񣬼��ٻ��Ƶ��ôӶ������Ż�
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
		std::cout << scene->mNumMeshes;
		ProcessNode(scene->mRootNode, scene);
	}

	void ProcessNode(aiNode* node, const aiScene* scene)
	{
		// ����ڵ����е���������еĻ���
		for (UINT i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessModel(i, mesh, scene);
		}
		// �������������ӽڵ��ظ���һ����
		for (UINT i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene);
		}
	}

	void ProcessModel(UINT index, aiMesh* mesh, const aiScene* scene)
	{
		Model& currModel = models.emplace_back();
		currModel.vertices.reserve(mesh->mNumVertices);
		currModel.indices.reserve(mesh->mNumFaces * 3);

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
				vertex.texCoord = {0.f,0.f};
			currModel.vertices.emplace_back(vertex);
		}
		
		// ��������
		for (UINT i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				currModel.indices.emplace_back(face.mIndices[j]);
		}
		
		// TODO: �������
		//if (mesh->mMaterialIndex >= 0)
		//{
		//}
	}

	std::span<Model const> GetModels()const { return models; }
};

#endif