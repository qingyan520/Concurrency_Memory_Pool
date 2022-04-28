#pragma once
#include<iostream>
#include <exception>
#include<vector>
#include<cstdlib>
#include<Windows.h>
#include<time.h>
#include<assert.h>
using std::cout;
using std::endl;
#define NFREE_LIST 208


class FreeList
{
private:
	void* _freeList;//自由链表的指针
public:
	//插入一块内存
	void Push(void* obj)
	{
		assert(obj);
		*(void**)obj= _freeList;
		_freeList = obj;
	}

	//弹出一块内存的首地址
	void* Pop()
	{
		assert(_freeList);
		void* obj = _freeList;
		_freeList = *(void**)_freeList;//取出_freeList保存的地址赋值给_freeList让_freeList进行后移
		return obj;
	}
};



class SizeClass{
public:
	size_t 
};