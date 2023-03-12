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

//存储整个场景加载过的texture，键为path
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



//一个模型在Assimp加载后可能会得到很多Mesh，比如人的头，身体，手等。不进行合并而逐一Draw就是现在默认管线采取的办法
//然而，ModelImporter这里可以还支持使用Meshlets，即采用Mesh Shader代替其他所有
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
	//path为从SolutionDir为开始的模型文件路径
	void Import(DXDevice* dxdevice, ID3D12GraphicsCommandList* cmdlist, std::string path)
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
		dxDevice = dxdevice;
		cmdList = cmdlist;
		ProcessNode( scene->mRootNode, scene);
	}

	void ProcessNode( aiNode* node, const aiScene* scene)
	{
		// 处理节点所有的网格（如果有的话）
		for (UINT i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			//ProcessModel(dxdevice, i, mesh, scene);
			ProcessModelMeshlet(i, mesh, scene);
		}
		// 接下来对它的子节点重复这一过程
		for (UINT i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode( node->mChildren[i], scene);
			//std::cout << "第"<<i<<"个Mesh完成拆分，共有"
		}
	}

	void ProcessModel( UINT index, aiMesh* mesh, const aiScene* scene)
	{
		Mesh& currMesh = model.emplace_back();
		currMesh.vertices.reserve(mesh->mNumVertices);
		currMesh.indices.reserve(mesh->mNumFaces * 3);
		UINT vertIndex = 0;
		bool ChongFu = false;
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

			//判断是否重复，并按逻辑插入顶点和顶点索引
			for (UINT iter = 0; iter<currMesh.vertices.size();iter++)
			{
				if (currMesh.vertices[iter] == vertex)
				{
					ChongFu = true;
					currMesh.indices.emplace_back(iter);
					break;
				}
			}
			//若不重复，插入
			if (!ChongFu)
			{
				currMesh.vertices.emplace_back(vertex);
				currMesh.indices.emplace_back(vertIndex++);	   //实际上vertIndex等于currMesh.vertices.size()-1
			}
			ChongFu = false;
		}
		//// 每个mesh有一个材质
		//// 加载纹理，Assimp不提供加载纹理的功能，需要额外用stb_image来加载
		//if (mesh->mMaterialIndex >= 0)
		//{
		//	//aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		//	////每个材质对象的内部对每种纹理类型都存储了一个纹理位置数组。不同的纹理类型都以aiTextureType_为前缀
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

			//1. 处理顶点数组和索引数组。
			//判断Vertex是否重复，并按逻辑插入顶点和顶点索引
			for (UINT iter = 0; iter < vertIndex;iter++)
			{
				if (currMesh.vertices[iter] == vertex)
				{
					ChongFu = true;
					currMesh.indices.emplace_back(iter);
					break;
				}
			}
			//若不重复，插入
			if (!ChongFu)
			{
				currMesh.vertices.emplace_back(vertex);
				currMesh.indices.emplace_back(vertIndex++);	   //实际上vertIndex等于currMesh.vertices.size()-1
			}
			ChongFu = false;
		}

		//{
		//	std::cout << "最初的index buffer数量为："<< currMesh.indices.size() << std::endl;
		//}

		//2. 处理Meshlet
		std::vector<UINT> newIndices;
		std::vector<UINT> primitiveIndiceBig;
		UINT indiceIndex = 0;

		//先更新一下vertex indice和primitive indice（暂且每个primitive indice用一个UINT来装)
		for (UINT i = 0;i < currMesh.indices.size(); i++)
		{
			UINT presentIndice = currMesh.indices[i];
			//判断Indice是否重复
			for (UINT iter = 0; iter < indiceIndex;iter++)
			{
				if (newIndices[iter] == presentIndice)
				{
					ChongFu = true;
					primitiveIndiceBig.emplace_back(iter);		//primitiveIndiceBig中放入的是vertexIndice的下标
					break;
				}
			}
			//若不重复，插入
			if (!ChongFu)
			{
				newIndices.emplace_back(presentIndice);
				primitiveIndiceBig.emplace_back(indiceIndex++);	   //实际上vertIndex等于currMesh.vertices.size()-1
			}
			ChongFu = false;
		}
		//{
		//	std::cout << "更改前的PrimitiveIndice为：" << std::endl;
		//	for (auto x : primitiveIndiceBig)
		//	{
		//		std::cout << x << " ";
		//	}
		//}

		//此时vertex indice已去重（但是这个vertex indice并不是我们要的，我们要的是下面的vertexInMeshlet），
		//也生成好了primitive indice（这个primitive indice也不是我们想要的，还得修改）。接着生成Meshlet
		//vertex indice走了64个或者primitive indice走了126*3个就停下来
		UINT vertexCount = 0;
		UINT vertexOffset = 0;
		UINT primitiveCount = 0;
		UINT primitiveOffset = 0;
		bool nextMeshlet = false;
		UINT NewVertInPrim = 0;
		std::vector<UINT> vertexInMeshlet;	//vertex indices
		UINT originPrimIndic[3];
		//std::cout << std::endl << "更改后的PrimitiveIndice：" << std::endl;
		for (UINT i = 0;i < primitiveIndiceBig.size();i+=3)
		{
			nextMeshlet = false;
			NewVertInPrim = 0;
			for (UINT j = 0;j < 3;j++)
			{
				originPrimIndic[j] = primitiveIndiceBig[i + j];
				// Primitive Indice中如果指向一个在该Meshlet中没出现过的vertex，则vertexcount++，并放入vertexInMeshlet
				for (UINT k = vertexOffset; k < vertexInMeshlet.size();k++)
				{
					//如果该vertex已经出现过，标记一下
					if (vertexInMeshlet[k] == newIndices[primitiveIndiceBig[i + j]])
					{
						primitiveIndiceBig[i + j] = k;	//
						//std::cout << primitiveIndiceBig[i + j] << " ";
						ChongFu = true;
						break;
					}
				}
				// 若没出现过，插入该vertex，vertexCount加一
				if (!ChongFu)
				{
					vertexCount++;
					NewVertInPrim++;
					//如果加上这个顶点变成65个顶点了，那么去掉这个三角图元，直接生成Meshlet，并重新开始处理下一个Meshlet
					if (vertexCount == 65)
					{
						// 生成Meshlet
						Meshlet meshlet;
						// 如果这个图元中有一个新顶点，VertCount改为64。如果有两个则改为63，三个则62
						meshlet.VertCount = 65 - NewVertInPrim;
						// 如果这个图元中有一个新顶点，vertexInMeshlet不用改。两个则去掉最后一个元素，三个则去掉最后两个元素
						vertexInMeshlet.resize(vertexInMeshlet.size() - NewVertInPrim + 1);
						// 同时，primitiveIndiceBig中对于该图元来说已经修改过的内容要改回去
						for (UINT k = 0; k <= j;k++)
							primitiveIndiceBig[i + k] = originPrimIndic[k];
						meshlet.VertOffset = vertexOffset;
						meshlet.PrimitiveCount = primitiveCount;
						meshlet.PrimitiveOffset = primitiveOffset;
						currMesh.meshlets.emplace_back(meshlet);

						//{
						//	std::cout << "Meshlet" << currMesh.meshlets.size() - 1 << "的信息为：" << std::endl;
						//	std::cout << "VertCount：" << meshlet.VertCount << "，VertOffset：" << meshlet.VertOffset <<
						//		"，PrimitiveCount：" << meshlet.PrimitiveCount << "，PrimitiveOffset：" << meshlet.PrimitiveOffset << std::endl << std::endl;
						//}
						//if (currMesh.meshlets.size() - 1 == 85)
						//	std::cout << "在这停顿" << std::endl;
						//if (currMesh.meshlets.size() - 1 == 48)
						//	std::cout << "在这停顿" << std::endl;
						//搞下一个Meshlet
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
				//生成Meshlet
				Meshlet meshlet;
				meshlet.VertCount = vertexCount;
				meshlet.VertOffset = vertexOffset;
				meshlet.PrimitiveCount = primitiveCount;
				meshlet.PrimitiveOffset = primitiveOffset;
				currMesh.meshlets.emplace_back(meshlet);

				//{
				//	std::cout << "Meshlet"<<currMesh.meshlets.size()-1<<"的信息为："<<std::endl;
				//	std::cout << "VertCount：" << meshlet.VertCount << "，VertOffset：" << meshlet.VertOffset <<
				//		"，PrimitiveCount：" << meshlet.PrimitiveCount << "，PrimitiveOffset：" << meshlet.PrimitiveOffset << std::endl << std::endl;
				//}
				//if (currMesh.meshlets.size() - 1 == 85)
				//	std::cout << "在这停顿" << std::endl;
				//if (currMesh.meshlets.size() - 1 == 48)
				//	std::cout << "在这停顿" << std::endl;
				//搞下一个Meshlet
				vertexCount = 0;
				primitiveCount = 0;
				primitiveOffset += meshlet.PrimitiveCount;
				vertexOffset += meshlet.VertCount;
			}
		}
		//把真·indice传进入
		currMesh.indices = vertexInMeshlet;

		//最后一步设置primitives：从uint3到uint，内存布局为2+10(2)+10(1)+10(0)。但要先注意primitive的值要先减去vertex offset
		for (auto& meshlet : currMesh.meshlets)
		{
			//if (meshlet.VertCount < 64)
			//	std::cout << "小于64" << std::endl;
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

		//输出一下VertIndices
		{
			//std::cout << "该Mesh的顶点数为：" << currMesh.vertices.size() << std::endl;
			//std::cout << "最终的primitiveIndices数量为：" <<currMesh.primitives.size() <<  std::endl; 
			//std::cout << "最终的VertexIndices为：" << std::endl; 
			//for (UINT ind = 0;ind < currMesh.indices.size();ind++)
			//{
			//	std::cout << currMesh.indices[ind] << " ";
			//}
			//std::cout << std::endl << "finish" << std::endl;
		}

		//最后の最后把当前的Mesh的meshletCount上传到constant buffer
		currMesh.UploadMeshletCountConstant(dxDevice, cmdList);
	}

	void GenerateBoundsMeshlet()
	{
		for (auto& mesh : model2)
		{
			for (auto& meshlet : mesh.meshlets)
			{
				//遍历顶点，计算顶点两个最值，拿到AABB
				DirectX::XMFLOAT3 maxFloat{-10000,-10000,-10000}, minFloat{ 10000,10000,10000 };
				//std::cout << "该meshlet的顶点为：" << std::endl;
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
				//std::cout << "该meshlet的最大最小值为："<<std::endl;
				//std::cout << maxFloat.x<< " " << maxFloat.y << " " << maxFloat.z << std::endl;
				//std::cout << minFloat.x<< " " << minFloat.y << " " << minFloat.z << std::endl;

				//根据AABB计算出BoundingSphere
				DirectX::XMFLOAT3 mid = { (maxFloat.x + minFloat.x) / 2,(maxFloat.y + minFloat.y) / 2,(maxFloat.z + minFloat.z) / 2 };
				DirectX::XMFLOAT3 minToMax = { maxFloat.x - minFloat.x, maxFloat.y - minFloat.y ,maxFloat.z - minFloat.z };
				float r = sqrt(minToMax.x * minToMax.x + minToMax.y * minToMax.y + minToMax.z * minToMax.z)/2;
				meshlet.boundingSphere.x = mid.x;
				meshlet.boundingSphere.y = mid.y;
				meshlet.boundingSphere.z = mid.z;
				meshlet.boundingSphere.w = r;
				//std::cout << "该meshlet的中心和半径为："<<std::endl;
				//std::cout << mid.x << " " << mid.y << " " << mid.z << " " << r << std::endl;
				//std::cout << std::endl;
			}
		}
	}

	std::span<const Mesh> GetMeshes()const { return model; }
	std::span<const Mesh2> GetMeshesMeshlet()const { return model2; }
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