#pragma once
#ifndef _VERTEXSTRUCTDESCRIBER_H_
#define _VERTEXSTRUCTDESCRIBER_H_

#include "../stdafx.h"
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <span>
#include <set>







//构造时自动记录类型信息，然后手动调用GetLayout来进行Parse
namespace rtti {
			
	//结构体在最下面噢~



	//一个Vertex含有一个InputElementData数组。这个数据结构记录了类型信息
	struct InputElementData 
	{	   
		enum class ScaleType : byte 
		{  //都是4字节
			Float,
			Int,
			UInt
		};
		ScaleType scale;
		byte dimension;
		uint semanticIndex;
		std::string semantic;
		size_t GetSize() const;
	};
	
	class ElementDescriber;
	//ElementStruct使用一个定义在cpp文件中的static全局变量充当单例。这个类的作用就是“容器存储 + Parser”
	class ElementStruct 
	{
		friend class ElementDescriber;
	private:
		//一种Vertex类型对应一个元素数组
		std::vector<InputElementData> variables;
	public:
		size_t structSize = 0;
		std::span<InputElementData const> Variables() const { return variables; }
		ElementStruct ();
		ElementStruct (ElementStruct const&) = delete;
		ElementStruct (ElementStruct&&) = default;
		
		//Parser到input_element_desc的接口
		void GetMeshLayout(uint slot, std::vector<D3D12_INPUT_ELEMENT_DESC>& resultVector) const;
		void InsertGlobalLayout(std::string name, std::vector<D3D12_INPUT_ELEMENT_DESC>& resultVector)const;
	};


	//构造时记录顶点布局到ElementStruct
	//ElementDecriber是ElementStruct的帮手类，前者的对象是一个
	class ElementDescriber 
	{
		//第一个InputElement的offset为零，第二个为dimension1*4，第三个为(dimension1+dimension2)*4......
		size_t offset;
	protected:
		//在构造函数中把这个InputElement描述给ElementStruct
		ElementDescriber(InputElementData&& elementData);
	public:
		size_t Offset() const { return offset; }
	};


	//为什么一定要有一个Describer和一个Template？准确地说是一定要有一个非泛型和泛型。
	//后者根据类型具体地实例化前者，前者非泛型才可以把函数定义在cpp文件中利用提前获取的ElementStruct指针来记录内容
	template<typename T>
	class ElementTemplate : public ElementDescriber
	{
	protected:
		ElementTemplate(InputElementData&& elementData) : ElementDescriber(std::move(elementData)) {}
	public:
		//给我一个指针确定基址，自动根据类型叠加偏移量，返回数据的引用
		const T& Get(const void* structPtr) const
		{
			size_t ptrNum = reinterpret_cast<size_t>(structPtr);
			return *reinterpret_cast<T const*>(ptrNum + Offset());
		}
		T& Get(void* structPtr) const 
		{
			size_t ptrNum = reinterpret_cast<size_t>(structPtr);
			return *reinterpret_cast<T*>(ptrNum + Offset());
		}
	};

	
	//Type是多包一层便于特化，Template有成员函数，直接特化代码重复过多
	template<typename T>
	struct ElementType {};
	template<>
	struct ElementType<int32> : public ElementTemplate<int32> 
	{
		ElementType(const char* semantic) : ElementTemplate<int32>(InputElementData{ InputElementData::ScaleType::Int, byte(1), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<uint> : public ElementTemplate<uint> 
	{
		ElementType(const char* semantic) : ElementTemplate<uint>(InputElementData{ InputElementData::ScaleType::UInt, byte(1), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<float> : public ElementTemplate<float>
	{
		ElementType(const char* semantic) : ElementTemplate<float>(InputElementData{ InputElementData::ScaleType::Float, byte(1), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<DirectX::XMFLOAT2> : public ElementTemplate<DirectX::XMFLOAT2> 
	{
		ElementType(const char* semantic) : ElementTemplate<DirectX::XMFLOAT2>(InputElementData{ InputElementData::ScaleType::Float, byte(2), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<DirectX::XMFLOAT3> : public ElementTemplate<DirectX::XMFLOAT3> 
	{
		ElementType(const char* semantic) : ElementTemplate<DirectX::XMFLOAT3>(InputElementData{ InputElementData::ScaleType::Float, byte(3), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<DirectX::XMFLOAT4> : public ElementTemplate<DirectX::XMFLOAT4> 
	{
		ElementType(const char* semantic) : ElementTemplate<DirectX::XMFLOAT4>(InputElementData{ InputElementData::ScaleType::Float, byte(4), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<DirectX::XMUINT2> : public ElementTemplate<DirectX::XMUINT2> 
	{
		ElementType(const char* semantic) : ElementTemplate<DirectX::XMUINT2>(InputElementData{ InputElementData::ScaleType::UInt, byte(2), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<DirectX::XMUINT3> : public ElementTemplate<DirectX::XMUINT3> 
	{
		ElementType(const char* semantic) : ElementTemplate<DirectX::XMUINT3>(InputElementData{ InputElementData::ScaleType::UInt, byte(3), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<DirectX::XMUINT4> : public ElementTemplate<DirectX::XMUINT4> 
	{
		ElementType(const char* semantic) : ElementTemplate<DirectX::XMUINT4>(InputElementData{ InputElementData::ScaleType::UInt, byte(4), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<DirectX::XMINT2> : public ElementTemplate<DirectX::XMINT2> 
	{
		ElementType(const char* semantic) : ElementTemplate<DirectX::XMINT2>(InputElementData{ InputElementData::ScaleType::Int, byte(2), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<DirectX::XMINT3> : public ElementTemplate<DirectX::XMINT3> 
	{
		ElementType(const char* semantic) : ElementTemplate<DirectX::XMINT3>(InputElementData{ InputElementData::ScaleType::Int, byte(3), uint(0), std::string(semantic) }) {}
	};
	template<>
	struct ElementType<DirectX::XMINT4> : public ElementTemplate<DirectX::XMINT4> 
	{
		ElementType(const char* semantic) : ElementTemplate<DirectX::XMINT4>(InputElementData{ InputElementData::ScaleType::Int, byte(4), uint(0), std::string(semantic) }) {}
	};


	struct Vertex : rtti::ElementStruct
	{
		rtti::ElementType<DirectX::XMFLOAT4> position = "POSITION";
		rtti::ElementType<DirectX::XMFLOAT3> normal = "NORMAL";
		rtti::ElementType<DirectX::XMFLOAT4> color = "COLOR";
		static Vertex& Instance()
		{
			static Vertex instance;
			return instance;
		}
	private:
		Vertex() {};
	};

	struct Vertex2 : rtti::ElementStruct
	{
		rtti::ElementType<DirectX::XMFLOAT4> position = "POSITION";
		rtti::ElementType<DirectX::XMFLOAT3> normal = "NORMAL";
		rtti::ElementType<DirectX::XMFLOAT2> texCoord = "TEXCOORD";
		static Vertex2& Instance()
		{
			static Vertex2 instance;
			return instance;
		}
	private:
		Vertex2() {};
	};
}// namespace rtti



#endif