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

//存储整个场景加载过的texture，键为path
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

//一个模型在Assimp加载后可能会得到很多Mesh，比如人的头，身体，手等。不进行合并而逐一Draw就是现在默认管线采取的办法
//然而，ModelImporter这里可以还支持使用Meshlets，即采用Mesh Shader代替其他所有
class ModelImporter
{
	std::vector<Mesh> model;
	std::vector<Meshlet> meshlets;
	std::string directory;
	Assimp::Importer importer;
	DXDevice* dxDevice;
	//std::vector<Texture*> textures;

public:
	//path为从SolutionDir为开始的模型文件路径
	void Import(DXDevice* dxdevice, std::string path)
	{
		//除了加载文件之外，Assimp允许我们设定一些选项来强制它对导入的数据做一些额外的计算或操作：
		//aiProcess_Triangulate：如果模型不是（全部）由三角形组成，它需要将模型所有的图元形状变换为三角形
		//aiProcess_FlipUVs：翻转y轴的纹理坐标
		//aiProcess_GenNormals：如果模型不包含法向量的话，就为每个顶点创建法线
		//aiProcess_SplitLargeMeshes：将比较大的网格分割成更小的子网格，如果你的渲染有最大顶点数限制，只能渲染较小的网格，那么它会非常有用
		//aiProcess_OptimizeMeshes：和上个选项相反，它会将多个小网格拼接为一个大的网格，减少绘制调用从而进行优化
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
		//文件夹路径就是模型路径去掉最后一个斜杠后剩下的部分
		directory = path.substr(0, path.find_last_of('/'));
		ProcessNode(dxdevice, scene->mRootNode, scene);
	}

	void ProcessNode(DXDevice* dxdevice, aiNode* node, const aiScene* scene)
	{
		// 处理节点所有的网格（如果有的话）
		for (UINT i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessModel(dxdevice, i, mesh, scene);
		}
		// 接下来对它的子节点重复这一过程
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
				vertex.texCoord = { 0.f,0.f };
			currMesh.vertices.emplace_back(vertex);
		}

		// 处理索引
		for (UINT i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				currMesh.indices.emplace_back(face.mIndices[j]);
		}

		// 每个mesh有一个材质
		// 加载纹理，Assimp不提供加载纹理的功能，需要额外用stb_image来加载
		if (mesh->mMaterialIndex >= 0)
		{
			//aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			////每个材质对象的内部对每种纹理类型都存储了一个纹理位置数组。不同的纹理类型都以aiTextureType_为前缀
			//loadMaterialTextures(dxdevice, material, aiTextureType_DIFFUSE, "texture_diffuse");
			//loadMaterialTextures(dxdevice, material, aiTextureType_SPECULAR, "texture_specular");
		}
	}

	std::span<const Mesh> GetMeshes()const { return model; }
};

//	//仅存放
//	//一个材质可以有很多纹理，一次拿取一种类型的纹理
//	void loadMaterialTextures(DXDevice* dxdevice, aiMaterial* mat, aiTextureType type, std::string typeName)
//	{
//		//该种纹理可以有很多个
//		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
//		{
//			aiString str;
//			mat->GetTexture(type, i, &str);
//			// 若该纹理已经加载到全局纹理数组中，直接拿
//			auto iter = textureLoaded.find(str.C_Str());
//			if (iter != textureLoaded.end())
//			{
//				textures.emplace_back(&textureLoaded[str.C_Str()]);
//				break;
//			}
//			// 如果纹理还没有被加载，先加载再拿
//			// 创建Texture，并使用stb_image parse数据
//			Texture texture = Texture(dxdevice);
//			textureLoaded.insert(str.C_Str(), texture); // 添加到已加载的纹理中
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