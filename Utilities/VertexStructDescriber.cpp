#include "VertexStructDescriber.h"
#include <string_view>
#include <unordered_map>
#include <iostream>

namespace rtti
{
	////记录全局的顶点输入布局
	std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> GlobalLayout;

	//curStruct指向当前正在构造的ElementStruct对象。
	//因为设计的是一个ElementStruct和一个ElementDescriber的模式，后者要对前者进行设置，修改等操作。前者必须提供一个接口供后者使用，
	//同时ElementStruct不唯一，所以不能使用单例。在cpp文件中定义一个static ElementStruct*而不是类内定义一个ElementStruct*，很巧妙地
	//让全局只存在一份可引用实例的同时，能应对多ElementStruct。因为ElementStruct的构造和ElementDescriber的构造一起算是一个“原子操作”
	static thread_local ElementStruct* curStruct = nullptr;

	size_t InputElementData::GetSize() const
	{
		return dimension * 4;
	}

	ElementStruct::ElementStruct()
	{
		//Vertex类必须继承ElementStruct类，这样才能最先构造一个ElementStruct从而使得curSturct有东西
		curStruct = this;
	}

	//构造成员时，把信息注册到ElementStruct* curStruct中
	ElementDescriber::ElementDescriber(InputElementData&& elementData)
	{
		//设置offset为目前struct已经积累的size
		offset = curStruct->structSize;
		if (!elementData.semantic.empty())
		{
			//从后往前扫，如果扫到了数字，就设置sementicIndex。rate是用来处理数字为两位数的情况的
			char const* ptr = elementData.semantic.data() + elementData.semantic.size() - 1;
			uint rate = 1;
			uint semanticIndex = 0;
			while (ptr != (elementData.semantic.data() - 1))
			{
				if (*ptr >= '0' && *ptr <= '9')
				{
					semanticIndex += rate * (int(*ptr) - int('0'));
					rate *= 10;
				}
				else
				{
					break;
				}
				--ptr;
			}
			auto leftSize = reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(elementData.semantic.data()) + 1;
			elementData.semantic.resize(leftSize);
			elementData.semanticIndex = semanticIndex;
		}
		//每构造一个成员（InputElement），就对structSize累加一个dimension*4，即该输入元素类型的大小
		curStruct->variables.emplace_back(std::move(elementData));
		curStruct->structSize += elementData.GetSize();
	}

	void ElementStruct::GetMeshLayout(uint slot, std::vector<D3D12_INPUT_ELEMENT_DESC>& resultVector) const
	{
		auto getFormat = [](const InputElementData & d) -> DXGI_FORMAT
		{
			switch (d.scale)
			{
			case InputElementData::ScaleType::Float:
				switch (d.dimension)
				{
				case 1:
					return DXGI_FORMAT_R32_FLOAT;
				case 2:
					return DXGI_FORMAT_R32G32_FLOAT;
				case 3:
					return DXGI_FORMAT_R32G32B32_FLOAT;
				case 4:
					return DXGI_FORMAT_R32G32B32A32_FLOAT;
				default:
					return DXGI_FORMAT_UNKNOWN;
				}
			case InputElementData::ScaleType::UInt:
				switch (d.dimension)
				{
				case 1:
					return DXGI_FORMAT_R32_UINT;
				case 2:
					return DXGI_FORMAT_R32G32_UINT;
				case 3:
					return DXGI_FORMAT_R32G32B32_UINT;
				case 4:
					return DXGI_FORMAT_R32G32B32A32_UINT;
				default:
					return DXGI_FORMAT_UNKNOWN;
				}
			case InputElementData::ScaleType::Int:
				switch (d.dimension)
				{
				case 1:
					return DXGI_FORMAT_R32_SINT;
				case 2:
					return DXGI_FORMAT_R32G32_SINT;
				case 3:
					return DXGI_FORMAT_R32G32B32_SINT;
				case 4:
					return DXGI_FORMAT_R32G32B32A32_SINT;
				default:
					return DXGI_FORMAT_UNKNOWN;
				}
			default:
				return DXGI_FORMAT_UNKNOWN;
			}
		};
		uint Offset = 0;
		for (size_t i = 0; i < variables.size(); ++i)
		{
			const rtti::InputElementData& element = variables[i];
			D3D12_INPUT_ELEMENT_DESC&     out	  = resultVector.emplace_back();   //描述顶点着色器的形参表
			out = { element.semantic.c_str(),						//sementic
					  uint(element.semanticIndex),					//sementicIndex：如TEXCOORD0和TEXCOORD1，相同的sementic用不同的index区分
					  getFormat(element),							//format：根据scale和dimension选择
					  slot,											//inputSlot：同样的思路：一个slot对应一个buffer。一种vertex中的所有element全走一个slot
					  Offset,										//alignByteOffset
					  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	//inputSlotClass：可选择该数据是per_vertex的还是per_instance的。
					  uint(0) };									//instanceDataStepRate：也与实例化有关
			Offset += element.GetSize();
		}
	}

	void ElementStruct::InsertGlobalLayout(std::string name , std::vector<D3D12_INPUT_ELEMENT_DESC>& resultVector) const
	{
		//set中没有时，插入set
		if (GlobalLayout.find(name) == GlobalLayout.end())
		{
			std::cout << "InsertGlobalLayoutSucceed" << std::endl;
			GlobalLayout.insert({ name, resultVector });
		}
	}
}