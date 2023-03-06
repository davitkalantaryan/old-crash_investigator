//
// file:			crash_investigator_linux_simple_analyze.cpp
// path:			src/core/crash_investigator_linux_simple_analyze.cpp
// created on:		2023 Mar 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


#define MEMORY_LEAK_ANALYZE_INIT_TIME_SEC   100
#define MEMORY_LEAK_ANALYZE_MAX_ALLOC       100


#define MEM_LEAK_ANALYZE_USE_STD_MUTEX

#include <crash_investigator/alloc_free.h>
#include <cinternal/hash/lhash.h>
#include <stack_investigator/investigator.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
#include <mutex>
#else
#include <pthread.h>
#endif


#ifdef CPPUTILS_GCC_FAMILY
#pragma GCC diagnostic ignored "-Wattributes"
#endif

#define MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS        16384
#define MEM_LEAK_ANALYZE_HASH_BY_STACK_BASKETS      4096

static size_t s_unMaxValue = 0;


struct CPPUTILS_DLL_PRIVATE SMemoryLeakAnalyseItem{
    struct StackInvestBacktrace*    pStack;
    size_t                          countForThisStack;
    CInternalLHashIterator          hashByStackIter;
};

static int s_nInitializationTimeNotPassed = 1;
static struct timeval       s_initTime;
#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
static ::std::mutex*        s_pMutexForHashes = CPPUTILS_NULL;
#else
static pthread_mutex_t      s_mutexForHashes;
#endif
static CinternalLHash_t     s_hashByAddress = CPPUTILS_NULL;
static CinternalLHash_t     s_hashByStack = CPPUTILS_NULL;

static void* MemoryLeakAnalyzerAddAllocedMem(int a_goBackInTheStackCalc, void* a_ptr);
static void MemoryLeakAnalyzerRemoveMemForFreeing(void* a_ptr);


static void* MemoryLeakAnalyzeMalloc(size_t a_size)
{
    void*const pRet = MemoryHandlerCLibMalloc(a_size);
    return MemoryLeakAnalyzerAddAllocedMem(1,pRet);
}


static void* MemoryLeakAnalyzeCalloc(size_t a_nmemb, size_t a_size)
{
    void*const pRet = MemoryHandlerCLibCalloc(a_nmemb,a_size);
    return MemoryLeakAnalyzerAddAllocedMem(1,pRet);
}


static void* MemoryLeakAnalyzeRealloc(void* a_ptr, size_t a_size)
{
    void*const pRet = MemoryHandlerCLibRealloc(a_ptr,a_size);

    if(pRet!=a_ptr){
        MemoryLeakAnalyzerRemoveMemForFreeing(a_ptr);
        return MemoryLeakAnalyzerAddAllocedMem(1,pRet);
    }

    return a_ptr;
}


static void MemoryLeakAnalyzeFree(void* a_ptr)
{
    MemoryLeakAnalyzerRemoveMemForFreeing(a_ptr);
    MemoryHandlerCLibFree(a_ptr);
}


static void* MemoryLeakAnalyzerAddAllocedMem(int a_goBackInTheStackCalc, void* a_ptr)
{
    static __thread int snIgnoreForThisThread = 0;
    size_t unMaxValue = 0;

    if(s_nInitializationTimeNotPassed){
        struct timeval currentTime;
        gettimeofday(&currentTime,CPPUTILS_NULL);
        if((currentTime.tv_sec-s_initTime.tv_sec)>MEMORY_LEAK_ANALYZE_INIT_TIME_SEC){
            s_nInitializationTimeNotPassed = 0;
        }
        else{
            return a_ptr;
        }
    }
    else if(snIgnoreForThisThread){
        return a_ptr;
    }

    snIgnoreForThisThread = 1;

    if(a_ptr){
        CInternalLHashIterator hashIterAdr, hashIterStack;
        size_t unHashAdr, unHashStack;
        const size_t cunAddress = (size_t)a_ptr;
        struct StackInvestBacktrace* pCurStack;
        struct SMemoryLeakAnalyseItem* pItem;

        pCurStack = StackInvestInitBacktraceDataForCurrentStack(a_goBackInTheStackCalc+1);
        if(!pCurStack){
            //MemoryHandlerCLibFree(pItem);
            MemoryHandlerCLibFree(a_ptr);
            return CPPUTILS_NULL;
        }

        {  // mutex lock region
#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
            ::std::lock_guard<::std::mutex> aGuard(*s_pMutexForHashes);
#else
            pthread_mutex_lock(&s_mutexForHashes);
#endif
            hashIterStack = CInternalLHashFindEx(s_hashByStack,CInternalAnyDataHPair(pCurStack),&unHashStack);
            if(hashIterStack){
                StackInvestFreeBacktraceData(pCurStack);
                pItem = (struct SMemoryLeakAnalyseItem*)(hashIterStack->data);
                if((++(pItem->countForThisStack))>MEMORY_LEAK_ANALYZE_MAX_ALLOC){
#ifndef MEM_LEAK_ANALYZE_USE_STD_MUTEX
                    pthread_mutex_unlock(&s_mutexForHashes);
#endif
                    fprintf(stderr,"!!!!!!!!!!!!!!!!!!!!!!!!!! possible memory leak!!!!!\n");
                    snIgnoreForThisThread = 0;
                    exit(1);
                }
                else if(pItem->countForThisStack>s_unMaxValue){
                    unMaxValue = s_unMaxValue = pItem->countForThisStack;
                }
            }
            else{
                pItem = (struct SMemoryLeakAnalyseItem*)MemoryHandlerCLibMalloc(sizeof(struct SMemoryLeakAnalyseItem));
                if(!pItem){
#ifndef MEM_LEAK_ANALYZE_USE_STD_MUTEX
                    pthread_mutex_unlock(&s_mutexForHashes);
#endif
                    StackInvestFreeBacktraceData(pCurStack);
                    MemoryHandlerCLibFree(a_ptr);
                    snIgnoreForThisThread = 0;
                    return CPPUTILS_NULL;
                }
                pItem->pStack = pCurStack;
                pItem->countForThisStack = 1;
                pItem->hashByStackIter = CInternalLHashAddDataWithKnownHash(s_hashByStack,pItem,CInternalAnyDataHPair(pCurStack),unHashStack);
            }

            hashIterAdr = CInternalLHashFindEx(s_hashByAddress,CInternalAnyDataHPair(cunAddress),&unHashAdr);
            assert(hashIterAdr==CPPUTILS_NULL);
            CInternalLHashAddDataWithKnownHash(s_hashByAddress,pItem,CInternalAnyDataHPair(cunAddress),unHashAdr);
#ifndef MEM_LEAK_ANALYZE_USE_STD_MUTEX
            pthread_mutex_unlock(&s_mutexForHashes);
#endif
        }  // end mutex lock region

    }  //  if(a_ptr){

    snIgnoreForThisThread = 0;

    if(unMaxValue>0){
        snIgnoreForThisThread = 1;
        printf("!!!!!!!!!!! new max =>  %d\n",(int)unMaxValue);
        snIgnoreForThisThread = 0;
    }

    return a_ptr;
}


static void MemoryLeakAnalyzerRemoveMemForFreeing(void* a_ptr)
{
    if(a_ptr){
        CInternalLHashIterator hashIterAdr;
        size_t unHashAdr;
        const size_t cunAddress = (size_t)a_ptr;

#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
        ::std::lock_guard<::std::mutex> aGuard(*s_pMutexForHashes);
#else
        pthread_mutex_lock(&s_mutexForHashes);
#endif
        hashIterAdr = CInternalLHashFindEx(s_hashByAddress,CInternalAnyDataHPair(cunAddress),&unHashAdr);
        //assert(hashIterAdr); // I think this should always be present
        // no because of initialization time, some memories are not here
        if(hashIterAdr){
            struct SMemoryLeakAnalyseItem* pItem;

            pItem = (struct SMemoryLeakAnalyseItem*)(hashIterAdr->data);
            CInternalLHashRemoveDataEx(s_hashByAddress,hashIterAdr);
            assert((pItem->countForThisStack)>0);
            if((--(pItem->countForThisStack))==0){
                struct StackInvestBacktrace*const pStack = pItem->pStack;
                CInternalLHashRemoveDataEx(s_hashByStack,pItem->hashByStackIter);
                StackInvestFreeBacktraceData(pStack);
            }
        }  //  if(hashIterAdr){

#ifndef MEM_LEAK_ANALYZE_USE_STD_MUTEX
        pthread_mutex_unlock(&s_mutexForHashes);
#endif

    }  //  if(a_ptr){
}


static size_t HashByAddressHasher(const void* a_key, size_t a_keySize)
{
    (void)a_keySize;
    return *((const size_t*)a_key);
}


typedef struct StackInvestBacktrace* ConstStackInvestBacktracePtr;


static size_t HashByStackHasher(const void* a_key, size_t a_keySize)
{
    (void)a_keySize;
    ConstStackInvestBacktracePtr*const ppStack = (ConstStackInvestBacktracePtr*)a_key;
    return StackInvestHashOfTheStack(*ppStack);
}


CPPUTILS_CODE_INITIALIZER(MemoryHandlerInit){
#ifndef MEM_LEAK_ANALYZE_USE_STD_MUTEX
    int nRet;
#endif

    s_nInitializationTimeNotPassed = 1;
    gettimeofday(&s_initTime,CPPUTILS_NULL);

#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
    s_pMutexForHashes = new ::std::mutex();
#else
    nRet = pthread_mutex_init(&s_mutexForHashes,CPPUTILS_NULL);
    if(nRet){
        exit(1);
    }
#endif

    s_hashByAddress = CInternalLHashCreateEx(MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS,&HashByAddressHasher,&MemoryHandlerCLibMalloc,&MemoryHandlerCLibFree);
    if(!s_hashByAddress){
#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
        delete s_pMutexForHashes;
#else
        pthread_mutex_destroy(&s_mutexForHashes);
#endif
        exit(1);
    }

    s_hashByStack = CInternalLHashCreateEx(MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS,&HashByStackHasher,&MemoryHandlerCLibMalloc,&MemoryHandlerCLibFree);
    if(!s_hashByStack){
        CInternalLHashDestroy(s_hashByAddress);
#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
        delete s_pMutexForHashes;
#else
        pthread_mutex_destroy(&s_mutexForHashes);
#endif
        exit(1);
    }

    MemoryHandlerSetMallocFnc(&MemoryLeakAnalyzeMalloc);
    MemoryHandlerSetCallocFnc(&MemoryLeakAnalyzeCalloc);
    MemoryHandlerSetReallocFnc(&MemoryLeakAnalyzeRealloc);
    MemoryHandlerSetFreeFnc(&MemoryLeakAnalyzeFree);

    // todo: use atexit family to regiter cleaner
}

