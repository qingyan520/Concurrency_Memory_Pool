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
	void* _freeList;//���������ָ��
public:
	//����һ���ڴ�
	void Push(void* obj)
	{
		assert(obj);
		*(void**)obj= _freeList;
		_freeList = obj;
	}

	//����һ���ڴ���׵�ַ
	void* Pop()
	{
		assert(_freeList);
		void* obj = _freeList;
		_freeList = *(void**)_freeList;//ȡ��_freeList����ĵ�ַ��ֵ��_freeList��_freeList���к���
		return obj;
	}
};



class SizeClass{
public:
	size_t 
};