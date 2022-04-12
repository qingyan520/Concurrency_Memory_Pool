#pragma once
#include<iostream>
#include <exception>
#include<vector>
#include<cstdlib>
using std::cout;
using std::endl;

template <class T>
class ObjectPool {
public:
	T* New()
	{
		T* obj = nullptr;
		if (_freeList)
		{
            void* next = *((void**)_freeList);
			obj =(T*)next;
		}

		return obj;
		if (sizeof(T) > _remainBytes)
		{
			_memory = (char*)malloc(128 * 1024);
			_remainBytes = 128 * 1024;
			if (_memory == nullptr)
			{
				throw std::bad_alloc();
			}
		}

		obj = (T*)_memory;
		new(obj)T;
		size_t objsize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
		_memory += objsize;
		_remainBytes -= objsize;
		return obj;

	}

	void Delete(T* obj)
	{
		obj->~T();
		*(void**)obj = _freeList;
		_freeList = obj;
	}
private:
	char* _memory;//申请一大块内存，指向这块内存的起始位置
	void* _freeList;//自由链表，连接返回的内存
	size_t _remainBytes;//记录剩余的字节数
};


struct TreeNode
{
    int _val;
    TreeNode* _left;
    TreeNode* _right;

    TreeNode() :_val(0),
        _left(nullptr),
        _right(nullptr)
    {}
};



//测试性能
void Test()
{
    //每一轮申请释放多少次
    const size_t Rounds = 3;
    const size_t N = 100000;
    size_t begin1 = clock();
    std::vector<TreeNode*>v1;
    v1.resize(N);
    for (size_t j = 0; j < Rounds; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            v1.push_back(new TreeNode);
        }
        for (int i = 0; i < N; ++i)
        {
            delete v1[i];
        }
        v1.clear();
    }
    size_t end1 = clock();
    ObjectPool<TreeNode> TNPool;
    size_t begin2 = clock();
    std::vector<TreeNode*> v2;
    v2.reserve(N);
    for (size_t j = 0; j < Rounds; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            v2.push_back(TNPool.New());
        }
        for (int i = 0; i < 100000; ++i)
        {
            TNPool.Delete(v2[i]);
        }
        v2.clear();
    }
    size_t end2 = clock();
    cout << "new cost time:" << end1 - begin1 << endl;
    cout << "object pool cost time:" << end2 - begin2 << endl;
}
