#pragma once
#ifndef _VERTEXSTRUCTDESCRIBER_H_
#define _VERTEXSTRUCTDESCRIBER_H_

#include "../stdafx.h"
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <span>
#include <set>







//����ʱ�Զ���¼������Ϣ��Ȼ���ֶ�����GetLayout������Parse
namespace rtti {
			
	//�ṹ������������~



	//һ��Vertex����һ��InputElementData���顣������ݽṹ��¼��������Ϣ
	struct InputElementData 
	{	   
		enum class ScaleType : byte 
		{  //����4�ֽ�
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
	//ElementStructʹ��һ��������cpp�ļ��е�staticȫ�ֱ����䵱���������������þ��ǡ������洢 + Parser��
	class ElementStruct 
	{
		friend class ElementDescriber;
	private:
		//һ��Vertex���Ͷ�Ӧһ��Ԫ������
		std::vector<InputElementData> variables;
	public:
		size_t structSize = 0;
		std::span<InputElementData const> Variables() const { return variables; }
		ElementStruct ();
		ElementStruct (ElementStruct const&) = delete;
		ElementStruct (ElementStruct&&) = default;
		
		//Parser��input_element_desc�Ľӿ�
		void GetMeshLayout(uint slot, std::vector<D3D12_INPUT_ELEMENT_DESC>& resultVector) const;
		void InsertGlobalLayout(std::string name, std::vector<D3D12_INPUT_ELEMENT_DESC>& resultVector)const;
	};


	//����ʱ��¼���㲼�ֵ�ElementStruct
	//ElementDecriber��ElementStruct�İ����࣬ǰ�ߵĶ�����һ��
	class ElementDescriber 
	{
		//��һ��InputElement��offsetΪ�㣬�ڶ���Ϊdimension1*4��������Ϊ(dimension1+dimension2)*4......
		size_t offset;
	protected:
		//�ڹ��캯���а����InputElement������ElementStruct
		ElementDescriber(InputElementData&& elementData);
	public:
		size_t Offset() const { return offset; }
	};


	//Ϊʲôһ��Ҫ��һ��Describer��һ��Template��׼ȷ��˵��һ��Ҫ��һ���Ƿ��ͺͷ��͡�
	//���߸������;����ʵ����ǰ�ߣ�ǰ�߷Ƿ��Ͳſ��԰Ѻ���������cpp�ļ���������ǰ��ȡ��ElementStructָ������¼����
	template<typename T>
	class ElementTemplate : public ElementDescriber
	{
	protected:
		ElementTemplate(InputElementData&& elementData) : ElementDescriber(std::move(elementData)) {}
	public:
		//����һ��ָ��ȷ����ַ���Զ��������͵���ƫ�������������ݵ�����
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

	
	//Type�Ƕ��һ������ػ���Template�г�Ա������ֱ���ػ������ظ�����
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