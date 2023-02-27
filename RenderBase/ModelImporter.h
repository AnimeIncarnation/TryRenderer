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
		//除了加载文件之外，Assimp允许我们设定一些选项来强制它对导入的数据做一些额外的计算或操作：
		//aiProcess_Triangulate：如果模型不是（全部）由三角形组成，它需要将模型所有的图元形状变换为三角形
		//aiProcess_FlipUVs：翻转y轴的纹理坐标
		//aiProcess_GenNormals：如果模型不包含法向量的话，就为每个顶点创建法线
		//aiProcess_SplitLargeMeshes：将比较大的网格分割成更小的子网格，如果你的渲染有最大顶点数限制，只能渲染较小的网格，那么它会非常有用
		//aiProcess_OptimizeMeshes：和上个选项相反，它会将多个小网格拼接为一个大的网格，减少绘制调用从而进行优化
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
		std::cout << scene->mNumMeshes;
		ProcessNode(scene->mRootNode, scene);
	}

	void ProcessNode(aiNode* node, const aiScene* scene)
	{
		// 处理节点所有的网格（如果有的话）
		for (UINT i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessModel(i, mesh, scene);
		}
		// 接下来对它的子节点重复这一过程
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

		// 复制顶点位置、法线和纹理坐标
		for (UINT i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
			vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
			// 网格是否有纹理坐标？
			if (mesh->mTextureCoords[0]) 
				vertex.texCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			else
				vertex.texCoord = {0.f,0.f};
			currModel.vertices.emplace_back(vertex);
		}
		
		// 处理索引
		for (UINT i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				currModel.indices.emplace_back(face.mIndices[j]);
		}
		
		// TODO: 处理材质
		//if (mesh->mMaterialIndex >= 0)
		//{
		//}
	}

	std::span<Model const> GetModels()const { return models; }
};

#endif