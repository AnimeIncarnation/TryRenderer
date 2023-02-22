#pragma once
#ifndef _BUFFERVIEW_H_
#define _BUFFERVIEW_H_

#include "../stdafx.h"
#include "Buffer.h"


struct BufferView
{
	const Buffer*	buffer = nullptr;
	UINT			size = 0;
	UINT			stride = 0;
	BufferView(const Buffer* buf, UINT si, UINT st) :buffer(buf), size(si), stride(st) {}
	BufferView(const Buffer* buf) :buffer(buf),size(buffer?buffer->GetSize():0) {}
	bool operator==(BufferView const& a) const 
	{
		return memcmp(this, &a, sizeof(BufferView)) == 0;
	}
	bool operator!=(BufferView const& a) const 
	{
		return !operator==(a);
	}
};


#endif