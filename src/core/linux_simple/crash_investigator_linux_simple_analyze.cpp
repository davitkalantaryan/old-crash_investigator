//
// file:			crash_investigator_linux_simple_analyze.cpp
// path:			src/core/crash_investigator_linux_simple_analyze.cpp
// created on:		2023 Mar 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


#include <crash_investigator/alloc_free.h>
#include <cinternal/hash/lhash.h>
#include <crash_investigator/core/backtrace.hpp>
#include <stdlib.h>
#include <assert.h>


#ifdef CPPUTILS_GCC_FAMILY
#pragma GCC diagnostic ignored "-Wattributes"
#endif

#define MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS        16384
#define MEM_LEAK_ANALYZE_HASH_BY_STACK_BASKETS      4096


struct CPPUTILS_DLL_PRIVATE SMemoryLeakAnalyseItem{
    void* pAddress;
};

static CinternalLHash_t    s_hashByAddress = CPPUTILS_NULL;
static CinternalLHash_t    s_hashByStack = CPPUTILS_NULL;


static void* MemoryLeakAnalyzeMalloc(size_t a_size)
{
    //
}


static void* MemoryLeakAnalyzeCalloc(size_t a_nmemb, size_t a_size)
{
    return (*g_calloc)(a_nmemb,a_size);
}


static void* MemoryLeakAnalyzeRealloc(void* a_ptr, size_t a_size)
{
    return MemoryHandlerRealloc(a_ptr,a_size);
}


static void MemoryLeakAnalyzeFree(void* a_ptr)
{
    MemoryHandlerFree(a_ptr);
}


static void* MemoryLeakAnalyzerAllocMem(int a_goBackInTheStackCalc, size_t a_size)
{
    void* pRet = MemoryHandlerCLibMalloc(a_size);
    if(pRet){
        CInternalLHashIterator hashIter;
        size_t unHash;
        const size_t cunAddress = (size_t)pRet;
        struct SMemoryLeakAnalyseItem*const pItem = (struct SMemoryLeakAnalyseItem*)MemoryHandlerCLibMalloc(sizeof(struct SMemoryLeakAnalyseItem));
        if(!pItem){
            return CPPUTILS_NULL;
        }
        pItem->pAddress = pRet;
        hashIter = CInternalLHashFindEx(s_hashByAddress,CInternalAnyDataHPair(cunAddress),&unHash);
        assert(hashIter==CPPUTILS_NULL);
        CInternalLHashAddDataWithKnownHash(s_hashByAddress,pItem,CInternalAnyDataHPair(cunAddress),unHash);
    }
    return pRet;
}


static size_t HashByAddressHasher(const void* a_key, size_t a_keySize)
{
    (void)a_keySize;
    return *((const size_t*)a_key);
}


CPPUTILS_CODE_INITIALIZER(MemoryHandlerInit){

    s_hashByAddress = CInternalLHashCreateEx(MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS,&HashByAddressHasher,&MemoryHandlerCLibMalloc,&MemoryHandlerCLibFree);
    if(!s_hashByAddress){
        exit(1);
    }

    s_hashByStack = CInternalLHashCreateEx(MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS,&HashByAddressHasher,&MemoryHandlerCLibMalloc,&MemoryHandlerCLibFree);
    if(!s_hashByStack){
        CInternalLHashDestroy(s_hashByAddress);
        exit(1);
    }

    MemoryHandlerSetMallocFnc(&MemoryLeakAnalyzeMalloc);
    MemoryHandlerSetCallocFnc(&MemoryLeakAnalyzeCalloc);
    MemoryHandlerSetReallocFnc(&MemoryLeakAnalyzeRealloc);
    MemoryHandlerSetFreeFnc(&MemoryLeakAnalyzeFree);

    // todo: use atexit family to regiter cleaner
}

