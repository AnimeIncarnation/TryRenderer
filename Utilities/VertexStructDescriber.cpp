#include "VertexStructDescriber.h"
#include <string_view>
#include <unordered_map>
#include <iostream>

namespace rtti
{
	////��¼ȫ�ֵĶ������벼��
	std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> GlobalLayout;

	//curStructָ��ǰ���ڹ����ElementStruct����
	//��Ϊ��Ƶ���һ��ElementStruct��һ��ElementDescriber��ģʽ������Ҫ��ǰ�߽������ã��޸ĵȲ�����ǰ�߱����ṩһ���ӿڹ�����ʹ�ã�
	//ͬʱElementStruct��Ψһ�����Բ���ʹ�õ�������cpp�ļ��ж���һ��static ElementStruct*���������ڶ���һ��ElementStruct*���������
	//��ȫ��ֻ����һ�ݿ�����ʵ����ͬʱ����Ӧ�Զ�ElementStruct����ΪElementStruct�Ĺ����ElementDescriber�Ĺ���һ������һ����ԭ�Ӳ�����
	static thread_local ElementStruct* curStruct = nullptr;

	size_t InputElementData::GetSize() const
	{
		return dimension * 4;
	}

	ElementStruct::ElementStruct()
	{
		//Vertex�����̳�ElementStruct�࣬�����������ȹ���һ��ElementStruct�Ӷ�ʹ��curSturct�ж���
		curStruct = this;
	}

	//�����Աʱ������Ϣע�ᵽElementStruct* curStruct��
	ElementDescriber::ElementDescriber(InputElementData&& elementData)
	{
		//����offsetΪĿǰstruct�Ѿ����۵�size
		offset = curStruct->structSize;
		if (!elementData.semantic.empty())
		{
			//�Ӻ���ǰɨ�����ɨ�������֣�������sementicIndex��rate��������������Ϊ��λ���������
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
		//ÿ����һ����Ա��InputElement�����Ͷ�structSize�ۼ�һ��dimension*4����������Ԫ�����͵Ĵ�С
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
			D3D12_INPUT_ELEMENT_DESC&     out	  = resultVector.emplace_back();   //����������ɫ�����βα�
			out = { element.semantic.c_str(),						//sementic
					  uint(element.semanticIndex),					//sementicIndex����TEXCOORD0��TEXCOORD1����ͬ��sementic�ò�ͬ��index����
					  getFormat(element),							//format������scale��dimensionѡ��
					  slot,											//inputSlot��ͬ����˼·��һ��slot��Ӧһ��buffer��һ��vertex�е�����elementȫ��һ��slot
					  Offset,										//alignByteOffset
					  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	//inputSlotClass����ѡ���������per_vertex�Ļ���per_instance�ġ�
					  uint(0) };									//instanceDataStepRate��Ҳ��ʵ�����й�
			Offset += element.GetSize();
		}
	}

	void ElementStruct::InsertGlobalLayout(std::string name , std::vector<D3D12_INPUT_ELEMENT_DESC>& resultVector) const
	{
		//set��û��ʱ������set
		if (GlobalLayout.find(name) == GlobalLayout.end())
		{
			std::cout << "InsertGlobalLayoutSucceed" << std::endl;
			GlobalLayout.insert({ name, resultVector });
		}
	}
}