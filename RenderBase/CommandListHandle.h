#pragma once
#ifndef _COMMANDLISTHANDLE_H_
#define _COMMANDLISTHANDLE_H_


#include "../stdafx.h"
#include "../d3dx12.h"

class CommandListHandle
{
	ID3D12GraphicsCommandList* cmdList;
	ID3D12CommandAllocator* cmdAlloc;
public:
	ID3D12GraphicsCommandList* CmdList() { return cmdList; }
	ID3D12CommandAllocator* CmdAlloc() { return cmdAlloc; }
	CommandListHandle();
	CommandListHandle(CommandListHandle const&) = delete;
	CommandListHandle(CommandListHandle&&);
	CommandListHandle& operator=(CommandListHandle&&);
	CommandListHandle(ID3D12CommandAllocator* allocator, ID3D12GraphicsCommandList* cmdList);
	~CommandListHandle();

};

#endif