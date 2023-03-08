//
// file:			crash_investigator_linux_simple_analyze.cpp
// path:			src/core/crash_investigator_linux_simple_analyze.cpp
// created on:		2023 Mar 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


#define MEMORY_LEAK_ANALYZE_INIT_TIME_SEC   100
#define MEMORY_LEAK_ANALYZE_MAX_ALLOC       600


#define MEM_LEAK_ANALYZE_USE_STD_MUTEX

#include <crash_investigator/alloc_free.h>
#include <cinternal/hash/lhash.h>
#include <stack_investigator/investigator.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
#include <mutex>
#else
#include <pthread.h>
#endif


static __thread int s_nLocksPerThreadCount = 0;

//#define MAKE_LOCK_STAT
class MemLeakAnMutexRaw : private ::std::mutex{
public:
    void lock(){
        if((++s_nLocksPerThreadCount)==1){
            ::std::mutex::lock();
        }
#ifdef MAKE_LOCK_STAT
        const pid_t cnPid = syscall(SYS_gettid);
        char vcName[100];
        vcName[0]=0;
        pthread_getname_np(pthread_self(),vcName, 99);
        printf("+++++++ locked thread=%d (name=\"%s\")\n",(int)cnPid,vcName);
#endif
    }
    void unlock(){
#ifdef MAKE_LOCK_STAT
        const pid_t cnPid = syscall(SYS_gettid);
        printf("------- unlocking thread=%d\n",(int)cnPid);
#endif
        if((--s_nLocksPerThreadCount)==0){
            ::std::mutex::unlock();
        }
    }
};

//typedef ::std::mutex  MemLeakAnMutex;
typedef MemLeakAnMutexRaw  MemLeakAnMutex;


#ifdef CPPUTILS_GCC_FAMILY
#pragma GCC diagnostic ignored "-Wattributes"
#endif

#define MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS        131072
#define MEM_LEAK_ANALYZE_HASH_BY_STACK_BASKETS      8192

static size_t s_unMaxValue = 0;


struct CPPUTILS_DLL_PRIVATE SMemoryLeakAnalyseItem{
    struct StackInvestBacktrace*    pStack;
    size_t                          countForThisStack;
    CInternalLHashIterator          hashByStackIter;
};

static int s_nInitializationTimeNotPassed = 1;
static struct timeval       s_initTime;
#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
static MemLeakAnMutex*       s_pMutexForHashes = CPPUTILS_NULL;
#else
static pthread_mutex_t      s_mutexForHashes;
#endif
static CinternalLHash_t     s_hashByAddress = CPPUTILS_NULL;
static CinternalLHash_t     s_hashByStack   = CPPUTILS_NULL;

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


static __thread int s_nIgnoreForThisThread = 0;

class Reseter{
public:
    ~Reseter(){
        s_nIgnoreForThisThread = 0;
    }
};


static void* MemoryLeakAnalyzerAddAllocedMem(int a_goBackInTheStackCalc, void* a_ptr)
{
    size_t unMaxValue = 0;

    if(s_nIgnoreForThisThread){
        return a_ptr;
    }
    else {
        s_nIgnoreForThisThread = 1;
        if(s_nInitializationTimeNotPassed){
            struct timeval currentTime;
            gettimeofday(&currentTime,CPPUTILS_NULL);
            if((currentTime.tv_sec-s_initTime.tv_sec)>MEMORY_LEAK_ANALYZE_INIT_TIME_SEC){
                s_nInitializationTimeNotPassed = 0;
            }
            else{
                s_nIgnoreForThisThread = 0;
                return a_ptr;
            }
        }
    }

    Reseter aReseter;


    if(a_ptr){
        CInternalLHashIterator hashIterAdr, hashIterStack;
        size_t unHashAdr, unHashStack;
        struct StackInvestBacktrace* pCurStack;
        struct SMemoryLeakAnalyseItem* pItem;

        pCurStack = StackInvestInitBacktraceDataForCurrentStack(a_goBackInTheStackCalc+1);
        if(!pCurStack){
            //MemoryHandlerCLibFree(pItem);
            MemoryHandlerCLibFree(a_ptr);
            s_nIgnoreForThisThread = 0;
            return CPPUTILS_NULL;
        }

        {  // mutex lock region
#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
            ::std::lock_guard<MemLeakAnMutex> aGuard(*s_pMutexForHashes);
#else
            pthread_mutex_lock(&s_mutexForHashes);
#endif
            hashIterStack = CInternalLHashFindEx(s_hashByStack,CInternalSmallIntHPair(pCurStack),&unHashStack);
            if(hashIterStack){
                StackInvestFreeBacktraceData(pCurStack);
                pItem = (struct SMemoryLeakAnalyseItem*)(hashIterStack->data);
                if((++(pItem->countForThisStack))>MEMORY_LEAK_ANALYZE_MAX_ALLOC){
#ifndef MEM_LEAK_ANALYZE_USE_STD_MUTEX
                    pthread_mutex_unlock(&s_mutexForHashes);
#endif
                    fprintf(stderr,"!!!!!!!!!!!!!!!!!!!!!!!!!! possible memory leak!!!!!\n");
                    s_nIgnoreForThisThread = 0;
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
                    s_nIgnoreForThisThread = 0;
                    return CPPUTILS_NULL;
                }
                pItem->pStack = pCurStack;
                pItem->countForThisStack = 1;
                pItem->hashByStackIter = CInternalLHashAddDataWithKnownHash(s_hashByStack,pItem,CInternalSmallIntHPair(pCurStack),unHashStack);
            }

            hashIterAdr = CInternalLHashFindEx(s_hashByAddress,CInternalSmallIntHPair(a_ptr),&unHashAdr);
            assert(hashIterAdr==CPPUTILS_NULL);
            CInternalLHashAddDataWithKnownHash(s_hashByAddress,pItem,CInternalSmallIntHPair(a_ptr),unHashAdr);
#ifndef MEM_LEAK_ANALYZE_USE_STD_MUTEX
            pthread_mutex_unlock(&s_mutexForHashes);
#endif

        }  // end mutex lock region

    }  //  if(a_ptr){


    if(unMaxValue>0){
        printf("!!!!!!!!!!! new max =>  %d\n",(int)unMaxValue);
    }

    {
        static int nCounter = 0;
        if((nCounter++ % 100000) == 0){
            printf("nCounter = %d\n",nCounter);
        }
    }

    s_nIgnoreForThisThread = 0;

    return a_ptr;
}


static void MemoryLeakAnalyzerRemoveMemForFreeing(void* a_ptr)
{
    if(a_ptr){
        CInternalLHashIterator hashIterAdr;
        size_t unHashAdr;

        if(!s_nIgnoreForThisThread){

#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
            //::std::lock_guard<MemLeakAnMutex> aGuard(*s_pMutexForHashes);
            s_pMutexForHashes->lock();
#else
            pthread_mutex_lock(&s_mutexForHashes);
#endif

        }
        hashIterAdr = CInternalLHashFindEx(s_hashByAddress,CInternalSmallIntHPair(a_ptr),&unHashAdr);
        // assert(hashIterAdr);
        // `assert` is not ok, because of initialization time, some memories are not here
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

        if(!s_nIgnoreForThisThread){
#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
            s_pMutexForHashes->unlock();
#else
            pthread_mutex_unlock(&s_mutexForHashes);
#endif
        }

    }  //  if(a_ptr){
}



static size_t HashByStackHasher(const void* a_key, size_t a_keySize)
{
    (void)a_keySize;
    StackInvestBacktrace*const pStack = (StackInvestBacktrace*)a_key;
    return StackInvestHashOfTheStack(pStack);
}


static bool IsTheSameStack(const void* a_key1, size_t a_keySize1, const void* a_key2, size_t a_keySize2)
{
    (void)a_keySize1;
    (void)a_keySize2;
    const StackInvestBacktrace*const pStack1 = (const StackInvestBacktrace*)a_key1;
    const StackInvestBacktrace*const pStack2 = (const StackInvestBacktrace*)a_key2;
    return StackInvestIsTheSameStack(pStack1,pStack2);
}


CPPUTILS_CODE_INITIALIZER(MemoryHandlerInit){
#ifndef MEM_LEAK_ANALYZE_USE_STD_MUTEX
    int nRet;
#endif

    s_nInitializationTimeNotPassed = 1;
    gettimeofday(&s_initTime,CPPUTILS_NULL);

#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
    s_pMutexForHashes = new MemLeakAnMutex();
#else
    nRet = pthread_mutex_init(&s_mutexForHashes,CPPUTILS_NULL);
    if(nRet){
        exit(1);
    }
#endif

    s_hashByAddress = CInternalLHashCreateExSmlInt(MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS,&MemoryHandlerCLibMalloc,&MemoryHandlerCLibFree);
    if(!s_hashByAddress){
#ifdef MEM_LEAK_ANALYZE_USE_STD_MUTEX
        delete s_pMutexForHashes;
#else
        pthread_mutex_destroy(&s_mutexForHashes);
#endif
        exit(1);
    }

    s_hashByStack = CInternalLHashCreateExAnyDefSmlInt(MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS,
                                                       &HashByStackHasher,&IsTheSameStack,
                                                       CPPUTILS_NULL,CPPUTILS_NULL,
                                                       &MemoryHandlerCLibMalloc,&MemoryHandlerCLibFree);
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

