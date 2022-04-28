#pragma once

#include"Common.h"

class ThreadCache
{
private:
	FreeList _freeLists[NFREE_LIST];
public:
	void* Allocate(size_t size);
};
