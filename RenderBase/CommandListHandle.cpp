#include "CommandListHandle.h"
#include "../DXSampleHelper.h"


CommandListHandle::CommandListHandle() :cmdList(nullptr), cmdAlloc(nullptr) {};
CommandListHandle::CommandListHandle(CommandListHandle&& input):cmdList(input.cmdList), cmdAlloc(input.cmdAlloc)
{
	input.cmdList = nullptr;
	input.cmdAlloc = nullptr;
}

CommandListHandle::CommandListHandle(ID3D12CommandAllocator* allocator, ID3D12GraphicsCommandList* cmdList)
{
	ThrowIfFailed(allocator->Reset());
	ThrowIfFailed(cmdList->Reset(allocator, nullptr));
}

CommandListHandle::~CommandListHandle()
{
	if (cmdList) 
		cmdList->Close();
}

CommandListHandle& CommandListHandle::operator=(CommandListHandle&& input)
{
	cmdList = input.cmdList;
	cmdAlloc = input.cmdAlloc;
	input.cmdList = nullptr;
	input.cmdAlloc = nullptr;
	return *this;
}