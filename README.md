### 高并发内存池

借鉴tcmalloc(ThreadCaching Malloc)，即线程缓存的malloc，实现了高效的多线程内存管理，用于替代系统的内存分配相关的函数(malloc，free)



池化技术：向系统先申请过量的资源，然后自己管理，以备不时之需，之所以申请过量资源，是因为每次申请资源都有较大的开销，那边不如提前申请好，提高程序运行效率

在计算机中除了内存池，还有连接池，线程池，对象池等，以线程池为例，它的主要思想是，先启动若干数量的线程，让他们处于睡眠状态，当接收到客户端请求时，唤醒某个沉睡的线程，让它处理客户端请求，当处理完请求之后，线程又进入了休眠状态

内存池：程序预先向系统申请一大块足够的内存，此后，当系统需要申请内存的时候，不是直接向操作习题申请，而是向内存池中申请，当释放的时候，不返回给操作系统，而是返回给内存池，当程序退出时，内存池才将申请的内存真正释放

内存池解决问题：

1.主要解决效率问题

2.内存碎片问题

malloc实际就是一个内存池





定长内存池

固定大小的内存申请管理

特定：

性能达到极致

不考虑内存碎片等问题

设计方式：

向内存申请一块足够大的内存块，然后每次申请内存时我们就切出去一小部分拿来使用，

```cpp
//例如
char*_memory=(char*)malloc(128*1024);//一次性直接申请128k的内存，然后申请内存时从这块内存切除对应的大小
int remainBytes=128*1024;//管理剩余的内存数量，单位是字节
```

注意：申请的使用使用char类型的指针(假设为_memory)进行申请，这样方便指针进行后移，例如：如果需要向这块大内存进行申请使用，那么我们就使得 _memory+=sizeof(int),进行后移

![image-20220412164324270](https://raw.githubusercontent.com/qingyan520/Cloud_img/master/img/image-20220412164324270.png)

使用时切出一小部分拿出来用：

![image-20220412164604192](https://raw.githubusercontent.com/qingyan520/Cloud_img/master/img/image-20220412164604192.png)

![image-20220412164841522](https://raw.githubusercontent.com/qingyan520/Cloud_img/master/img/image-20220412164841522.png)

```cpp
//对应伪代码
//假设我们有个T类型的obj指针前来申请对象
obj=_memory;
_memory+=sizeof(T);
_remainBytes-=sizeof(T);
```

当我们释放内存的时候不是直接还给操作系统，而是还给这块大内存，让这块内存被重新使用

![image-20220412165154994](https://raw.githubusercontent.com/qingyan520/Cloud_img/master/img/image-20220412165154994.png)

注意看上面的图：我们让所有还回来的内存像链表链表一样被连接起来，其中_freeList被称为头指针，它的前4个或者8个字节存储下一个内存单元的地址,假设有块内存obj被使用使用之后还回来，那么我们就利用头插法将obj作为头部， _freeList进行前移

```cpp
*(**obj)=_freeList;//注意*(**)obj可以取出obj的前4个或8个字节，然后让他存储下一个地址，即_freeList的地址
_freeList=obj;//让obj重新称为头
```

既然_freeList的前4个或者8个自己保存地址，那么我们必须保证这个obj必须能够存下这些地址，所以我们在申请的时候就要保证obj对象的大小，最起码要能存下一个4/8字节的指针大小，所以前面申请的代码就要进行略微改动

```cpp
//对应伪代码
//假设我们有个T类型的obj指针前来申请对象
obj=_memory;
int objsize=sizeof(T)<sizeof(void*)? sizeof(void*):sizeof(T);//判断这个对象大小是否大于sizoef(void*)
_memory+=objsize;
_remainBytes-=objsize;
```

既然我们已经用一个链表管理每次还回来的内存了，那么这些还回来的内存改怎么办呢？答案是重复利用，所以当我们申请内存时，如果_freeList不为空，就说明当前已经有还回来的内存了，那么我们优先使用这块内存来进行申请，节约空间

```cpp
//说明有还回来的内存，重复利用
if(_freeList)
{
    void*next=*(void**)_freeList;//保存_freeList保存的下一个位置的地址，然后让_freeList指向下一个位置
    obj=_freeList;//将当前位置赋值给obj
    _freeList=next;//_feeList指向下一个位置
    return obj;//返回这块内存进行使用
}
```

申请内存的完整代码

```cpp
    
	T*New()
    {
        //优先把还回来的内存块对象重复利用
        T*_obj==nullptr;
        
        //说明有还回来的内存，重复利用
        if(_freeList)
        {
            void*next=*(void**)_freeList;
            obj=_freeList;
            _freeList=next;
            return obj;
        }
        //剩余内存不够一个对象大小时，重新开一个大块空间
        if(_remainBytes<sizeof(T))
        {
            _memory=(char*)malloc(128*1024);            
            _remainBytes=128*1024;
            if(_memory==nullptr)
            {
                throw bad_alloc();
            }
        }
        
        _obj=(T*)_memory;
        size_t objsize=sizeof(T)<sizeof(void*)? sizeof(void*):sizeof(T);
       	_memory+=objsize;
       	_remainBytes-=objsize;
        //定位new,显示调用T的构造函数初始阿虎
        new(obj) T;
        return _obj;
    }
    
```

释放当前内存

```cpp
void Delete(T* obj)
{
    //显示调用T的析构函数
	obj->~T();
    if (obj != nullptr)
    {
        *(void**)obj = _freeList;
        _freeList = obj;
    }
}
```

上面的代码我们使用malloc，下面我们可以将malloc转换成系统调用函数申请空间，默认以页为单位进行内存申请，一页为4/8k,我们将所有的malloc函数替换为SystemAlloc函数即可，当然，这样做并不能提升效率，但是可以全部使用系统调用

```
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
    //windows平台下申请内存
    void* ptr = VirtualAlloc(0, kpage * 8 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    //Linux下使用brk,mmap等
#endif

    if (ptr == nullptr)
    {
        throw std::bad_alloc();
    }
    return ptr;
}

```

完整代码如下所示：

```cpp
#pragma once
#include<iostream>
#include<vector>
#include<Windows.h>
#include<time.h>
using std::cout;
using std::endl;

//向系统申请内存，以页为单位进行申请
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
    //windows平台下申请内存
    void* ptr = VirtualAlloc(0, kpage * 8 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    //Linux下使用brk,mmap等
#endif

    if (ptr == nullptr)
    {
        throw std::bad_alloc();
    }
    return ptr;
}


template <class T>
class ObjectPool {
public:
    //申请内存
	T* New()
	{
		T* obj = nullptr;
        //首先向_freeList中进行申请
		if (_freeList)
		{
            void* next = *((void**)_freeList);
			obj =(T*)_freeList;
            _freeList = next;
		}

		return obj;
        //判断剩余内存是否足够，如果不足够就申请一大块内存进行读取
		if (sizeof(T) > _remainBytes)
		{
			_memory = (char*)SystemAlloc(16);
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
        if (obj != nullptr)
        {
            *(void**)obj = _freeList;//obj
            _freeList = obj;
        }
	}
private:
	char* _memory=nullptr;//申请一大块内存，指向这块内存的起始位置
	void* _freeList=nullptr;//自由链表，连接返回的内存
	size_t _remainBytes=0;//记录剩余的字节数
};

```

性能测试

我们分三轮向内存中申请TreeNode节点，统计时间

```

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
    const size_t N = 10000000;
    size_t begin1 = clock();
    std::vector<TreeNode*>v1;
    v1.resize(N);
    for (size_t j = 0; j < Rounds; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            v1.push_back(new TreeNode);
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
        v2.clear();
    }
    size_t end2 = clock();
    cout << "new cost time:" << end1 - begin1 << endl;
    cout << "object pool cost time:" << end2 - begin2 << endl;
}
```

![image-20220412174331904](https://raw.githubusercontent.com/qingyan520/Cloud_img/master/img/image-20220412174331904.png)

![image-20220412174354746](https://raw.githubusercontent.com/qingyan520/Cloud_img/master/img/image-20220412174354746.png)

![image-20220412174419729](https://raw.githubusercontent.com/qingyan520/Cloud_img/master/img/image-20220412174419729.png)

接下来申请后进行释放

```
    const size_t Rounds = 3;
    const size_t N = 100000000;
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
```

![image-20220412175559457](https://raw.githubusercontent.com/qingyan520/Cloud_img/master/img/image-20220412175559457.png)



通过上述结果不难看出定长内存池几乎比malloc快了2倍左右



```

```

#### 高并发内存池整体框架设计

考虑下面几方面问题：

1.性能问题

2.多线程环境下，锁竞争问题

3.内存碎片问题

高并发内存池主要由以下3个部分构成：

1.thread cache:线程缓存是每个线程独有的，用于小于256K的内存的分配，线程从这里申请内存不需要加锁，每个线程独享一个cache，这也就是这个并发线程池高效的地方

2.central cache:中心缓存是所有线程所共享的，thread_cache是按需从central cache中获取的对象，central cache合适的时机回收thread cache中的对象，避免一个线程占用了太多的内存，而其它线程的内存吃紧，达到内存分配在多个线程中更均衡的按需调度的目的(central cache 是存在竞争的，所以从这里取内存对象是需要加锁的，首先这里用的是桶锁，其次只有thread cache的没有内存对象时才会找central cache，所以这里的竞争不会很激烈)

3.page cache:页缓存时在central cache 缓存上面的一层缓存，存储的内存是以页为单位存储及分配的，central cache没有内存对象时，从page cache分配出一定数量的page，并切割成定长大小的内存，分配给central cache，当一个span的及格跨度也的对象都回收以后，page cache会回收central cache满足条件的span对象，并且合并相邻的页，组成更大的页，缓解内存碎片的问题



thread cache 设计

thread cache是一个哈希桶结构，每个桶是一个按桶位置映射的大小的内存块对象的自由链表，每个线程都会有一个thread cache对象，这样每个线程在这里获取对象和释放对象时都是无锁的

###### Common.h

```cpp
//管理小对象切分好的小对象的自由链表
#include<assert.h>
#include<thread>
#include<mutex>
static const size_t MAX_BYTES=256*1024;
static const size_t NFREE_LITS=208;
void*& NextObj(void*obj)
{
    return *(void**)obj;
}

//管理小对象切分好的自由链表
class FreeList
{
    public:
    //插入一个对象
    void Push(void*obj)
    {
        assert(obj);
        //头插
        *(void**)obj=_freeList;
        _freeList=obj;
    }
    
    //弹出一个对象
    void*Pop()
    {
        assert(_freeList);
        //头删
        void*next=*(void**)_freeList;
        
        _freeList=next;
    }
    
    bool Empty()
    {
        return _freeList==nullptr;
    }
    private:
    void*_freeList;
};


//计算对象大小的对其映射规则
class SizeClass
{
    public:
    //整体控制在最多10%左右的内碎片浪费
    //[1,128]               8字节对齐       freelist[1,16)
    //[128+1,1024]          16字节对齐      freelist[16,72)
    //[1024+1,8*1024]       128字节对齐     freelist[72,128)
    //[8*1024+1,64*1024]    1024字节对齐    freelist[128,184)
    //[64*1024+1,256*1024]  8*1014字节对齐  freelist[184,208)
   static inline size_t _RoundUp(size_t size,size_t alignNum)
    {
        size_t alignSize=size;//对齐数
        if(size%alignNum!=0)
        {
            alignSize=(size/alignNum+1)*alignNum;
        }
        else
        {
            alignSize=size;
        }
       return alignSize;
    }
    size_t RoundUp(size_t size)
    {
        if(size<=128)
        {
            return _RoundUp(size,8);
        }
        else if(size<=1024)
        {
            return _RoundUp(size,16);
        }
        else if(size<=8*1024)
        {
            return _RoundUp(size,128);
        }
        else if(size<=64*1024)
        {
            return _RoundUp(size,1024);
        }
        else if(size<=256*1024)
        {
            return _RoundUp(size,8*1024);
        }
        else
        {
            assert(false);
            return -1;
        }
    }
    
    static inline size_t _Index(size_t bytes,size_t allignNum)
    {
        //return ((bytes+(1<<align_shift)-1)>>align_shift)-1;
        if(bytes%alignNum==0)
        {
            return bytes/alignNum-1;
        }
        else
        {
            return bytes/alignNum;
        }
    }
    
    static inline size_t Index(size3_t bytes)
    {
        assert(bytes<=MAX_BYTES);
        static int group_array[4]={16,56,56,56};
        if(bytes<=128)
        {
            return _Index(bytes,3);
        }
        else if(bytes<=1024)
        {
            return _Index(bytes-128,4)+group_array[0];
        }
        else if(bytes<=8*1024)
        {
            return _Index(bytest-1024,4)+group_array[0];
        }
        else if(bytes<=64*1024)
        {
            return _Index(bytes-8*1024,10)+group_array[2]+group_array[1]+group_array[0];
        }
        else if(bytes<=256*1024)
        {
            return _Index(bytes-64*1024,13)+group_array[3]+group_array[2]
                +group_array[1]+group_array[0];
        }
        else
        {
            assert(false);
            return -1;
        }
    }
    
};
```



###### ThreadCache.h

```cpp
//ThreadCache.h
#include"Common.h"
class ThreadCache{
    public:
    void*Allocate(size_t size);//申请空间
    void Deallocate(vaoid*ptr,size_t size);//释放空间
    void*FetchFromCentralCache(size_t index,size_t size);//从中心缓冲获取对象
    
    private:
    FreeList _freeLists[NFREE_LISTS];
};

static _declspec(thread) ThreadCache* pTLSThreadCache=nullptr;
```

###### ThreadCache.cpp

```cpp
#include "HreadCache.h"
void*ThreadCache::Allocate(size_t size)
{
    assert(size<=MAX_BYTES);
    size_t alignSize=SizeClass::RoundUp(size);//对齐数
    size_t index=SizeClaaa::Index(size);//判断在那个桶里面
    
    if(!_freeLists[index].Empty())
    {
        return _freeLists[index].Pop();
    }
    else
    {
        return FetchFromCentralCache(index,alignSize);
    }
}

void ThreadCache::Deallocate(void*ptr,size_t size)
{
    assert(ptr);
    assert(size<=MAX_BYTES);
    //找到映射的自由链表位置插入
    size_t index=SizeClass::Index(size);
    
    _freeLists[index].Push(ptr);
}

void*ThreadCache::FetchFromCentralCache(size_t index,size_t size)
{
    return nullptr;
}
```

哈希桶内存映射关系



TLS-thread local storage



###### ConcurrentAlloc.h

```cpp
#include"Common.h"
#include"ThreadCache.h"

static void*ConcurrentAlloc(size_t size)
{
    //通过TLS，每个线程无锁的获取自己的对象
    if(pTSLThreadCache==nullptr)
    {
        pTSLThreaadCache=new ThreadCache;
    }
    return pTSLThreadCache->Allocate(size);
}

static void*ConcurrentFree(void*ptr,zie_t size)
{
    assert(pTLSThreadCache);
    pTSLThreadCache->Deallocate(ptr);
}
```



###### UnitTest.cpp

```cpp
#include"ObjectPool.h"
#include"ConcurrentAlloc.h"
void Alloc1()
{
    for(int i=0;i<5;i++)
    {
        void*ptr=ConcurrAlloc(6);
        
    }
}

void Alloc2()
{
    std::thread t1(Alloc1);
    std::thread t2(Alloc2);
    
    t1.join();
    t2.join();
}

void TLSTest()
{
    
}

int main()
{
    
}
```
