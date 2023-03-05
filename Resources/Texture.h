//#pragma once
//#ifndef _TEXTURE_H_
//#define _TEXTURE_H_
//#include "Resource.h"
//#include "../RenderBase/DXDevice.h"
//#include "DefaultBuffer.h"
//#include "UploadBuffer.h"
//
//
//enum class TextureDimension : UINT
//{
//	None,
//	Tex1D,
//	Tex2D,
//	Tex3D,
//	Cubemap,
//	Tex2DArray,
//};
//
////纹理：
////①纹理格式Format：指定一个纹素的内容
////②纹理维度Dimension
////③纹理用途Usage：SRV
//class Texture: Resource
//{
//public:
//	enum class TextureUsage : UINT
//	{
//		None,
//		RenderTarget,
//		DepthStencil,
//		ShaderResource,
//		UnorderedAccess
//	};
//
//private:
//	std::unique_ptr<DefaultBuffer> texture;
//	std::unique_ptr<UploadBuffer> uploadSupport;
//
//public:
//	std::string path;
//	std::string name;
//	Texture(DXDevice* device) :Resource(device) {}
//	Texture(const Texture&) = delete;
//	Texture& operator=(const Texture&) = delete;
//	Texture(Texture&&) = default;
//
//	void LoadDDSTexture(ID3D12GraphicsCommandList* cmdList, std::string path, std::string name);
//
//
//};
//
//#endif
//
