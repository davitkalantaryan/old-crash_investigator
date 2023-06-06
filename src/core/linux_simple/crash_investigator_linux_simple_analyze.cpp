//
// file:			crash_investigator_linux_simple_analyze.cpp
// path:			src/core/crash_investigator_linux_simple_analyze.cpp
// created on:		2023 Mar 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


#define MEMORY_LEAK_ANALYZE_INIT_TIME_SEC   20
#define MEMORY_LEAK_ANALYZE_MAX_ALLOC       300


#define MEM_LEAK_ANALYZE_USE_STD_MUTEX

#include <crash_investigator/alloc_free.h>
#include <cinternal/hash/dllhash.h>
#include <stack_investigator/investigator.h>
#include <mutex>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif
#ifdef MAKE_LOCK_STAT
#include <sys/syscall.h>
#include <sys/types.h>
#endif

#ifdef _WIN32
#define ctime_s_t(_timep,_buffer,_numberOfElements)     ctime_s(_buffer,_numberOfElements,_timep)
#else
#define ctime_s_t(_timep,_buffer,_numberOfElements)     CPPUTILS_STATIC_CAST(void,_numberOfElements);ctime_r(_timep,_buffer)
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
    CinternalDLLHashItem_t          hashByStackIter;
};

static int s_nInitializationTimeNotPassed = 1;
static time_t               s_initTimeSec = 0;
static MemLeakAnMutex*      s_pMutexForHashes = CPPUTILS_NULL;
static CinternalDLLHash_t   s_hashByAddress = CPPUTILS_NULL;
static CinternalDLLHash_t   s_hashByStack   = CPPUTILS_NULL;

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


#ifdef MEM_HANDLER_MMAP_NEEDED

static void* MemoryLeakAnalyzeMmap(void* a_addr, size_t a_len, int a_prot, int a_flags,int a_fildes, off_t a_off)
{
    void* pRet;
    ++s_nIgnoreForThisThread;
    pRet = MemoryHandlerCLibMmap(a_addr,a_len,a_prot,a_flags,a_fildes,a_off);
    printf("mmap called!\n");
    fflush(stdout);
    //return MemoryLeakAnalyzerAddAllocedMem(1,pRet);
    --s_nIgnoreForThisThread;
    return pRet;
}

#endif  //  #ifdef MEM_HANDLER_MMAP_NEEDED


class Reseter{
public:
    ~Reseter(){
        --s_nIgnoreForThisThread;
    }
};


static void* MemoryLeakAnalyzerAddAllocedMem(int a_goBackInTheStackCalc, void* a_ptr)
{
    size_t unMaxValue = 0;

    if(s_nIgnoreForThisThread>0){
        return a_ptr;
    }
    else {
        ++s_nIgnoreForThisThread;
        if(s_nInitializationTimeNotPassed){
            time_t currentTime;
            currentTime = time(&currentTime);
            if((currentTime - s_initTimeSec)>MEMORY_LEAK_ANALYZE_INIT_TIME_SEC){
                s_nInitializationTimeNotPassed = 0;
            }
            else{
                --s_nIgnoreForThisThread;
                return a_ptr;
            }
        }
    }

    Reseter aReseter;


    if(a_ptr){
        CinternalDLLHashItem_t hashIterAdr, hashIterStack;
        size_t unHashAdr, unHashStack;
        struct StackInvestBacktrace* pCurStack;
        struct SMemoryLeakAnalyseItem* pItem;

        pCurStack = StackInvestInitBacktraceDataForCurrentStack(a_goBackInTheStackCalc+1);
        if(!pCurStack){
            //MemoryHandlerCLibFree(pItem);
            MemoryHandlerCLibFree(a_ptr);
            return CPPUTILS_NULL;
        }

        {  // mutex lock region
            ::std::lock_guard<MemLeakAnMutex> aGuard(*s_pMutexForHashes);

            hashIterStack = CInternalDLLHashFindEx(s_hashByStack,pCurStack,0,&unHashStack);
            if(hashIterStack){
                StackInvestFreeBacktraceData(pCurStack);
                pItem = (struct SMemoryLeakAnalyseItem*)(hashIterStack->data);
                if((++(pItem->countForThisStack))>MEMORY_LEAK_ANALYZE_MAX_ALLOC){
                    char* pcTemp;
                    char vcCurTimeStr[128];
                    time_t currentTime;

                    currentTime = time(&currentTime);
                    ctime_s_t(&currentTime,vcCurTimeStr,127);
                    pcTemp = strchr(vcCurTimeStr,'\n');
                    if(pcTemp){
                        *pcTemp = 0;
                        fprintf(stderr,"!!! %s",vcCurTimeStr);
                    }
                    else{
                        fprintf(stderr,"!!!!!!!");
                    }

                    fprintf(stderr,"  possible memory leak!!!!!\n");
                    fflush(stderr);
                    //StackInvestPrintTrace();
                    exit(1);
                }
                else if(pItem->countForThisStack>s_unMaxValue){
                    unMaxValue = s_unMaxValue = pItem->countForThisStack;
                }
            }
            else{
                pItem = (struct SMemoryLeakAnalyseItem*)MemoryHandlerCLibMalloc(sizeof(struct SMemoryLeakAnalyseItem));
                if(!pItem){
                    StackInvestFreeBacktraceData(pCurStack);
                    MemoryHandlerCLibFree(a_ptr);
                    return CPPUTILS_NULL;
                }
                pItem->pStack = pCurStack;
                pItem->countForThisStack = 1;
                pItem->hashByStackIter = CInternalDLLHashAddDataWithKnownHash(s_hashByStack,pItem,pCurStack,0,unHashStack);
            }

            hashIterAdr = CInternalDLLHashFindEx(s_hashByAddress,CInternalSmallIntHPairFn(a_ptr),&unHashAdr);
            assert(hashIterAdr==CPPUTILS_NULL);
            CInternalDLLHashAddDataWithKnownHash(s_hashByAddress,pItem,CInternalSmallIntHPairFn(a_ptr),unHashAdr);

        }  // end mutex lock region

    }  //  if(a_ptr){


    if(unMaxValue>0){
        char* pcTemp;
        char vcCurTimeStr[128];
        time_t currentTime;
        currentTime = time(&currentTime);
        ctime_s_t(&currentTime,vcCurTimeStr,127);
        pcTemp = strchr(vcCurTimeStr,'\n');
        if(pcTemp){
            *pcTemp = 0;
            printf("!!! %s",vcCurTimeStr);
        }
        else{
            printf("!!!!!!!");
        }
        printf(" new max =>  %d\n",(int)unMaxValue);
        fflush(stdout);
        //StackInvestPrintTrace();
    }

    return a_ptr;
}


static void MemoryLeakAnalyzerRemoveMemForFreeing(void* a_ptr)
{
    if(a_ptr){
        CinternalDLLHashItem_t hashIterAdr;
        size_t unHashAdr;

        ++s_nIgnoreForThisThread;
        Reseter aReseter;

        ::std::lock_guard<MemLeakAnMutex> aGuard(*s_pMutexForHashes);

        hashIterAdr = CInternalDLLHashFindEx(s_hashByAddress,CInternalSmallIntHPairFn(a_ptr),&unHashAdr);
        // assert(hashIterAdr);
        // `assert` is not ok, because of initialization time, some memories are not here  // this is not correct
        // some memories are not here because when locking done with s_nIgnoreForThisThread, then it is not inserted
        if(hashIterAdr){
            struct SMemoryLeakAnalyseItem* pItem;

            pItem = (struct SMemoryLeakAnalyseItem*)(hashIterAdr->data);
            CInternalDLLHashRemoveDataEx(s_hashByAddress,hashIterAdr);
            assert((pItem->countForThisStack)>0);
            if((--(pItem->countForThisStack))==0){
                struct StackInvestBacktrace*const pStack = pItem->pStack;
                CInternalDLLHashRemoveDataEx(s_hashByStack,pItem->hashByStackIter);
                StackInvestFreeBacktraceData(pStack);
            }
        }  //  if(hashIterAdr){
        else{
            if(!s_nInitializationTimeNotPassed){
                //printf("No memory found for the pointer %p\n",a_ptr);
                //fflush(stdout);
            }
        }

    }  //  if(a_ptr){
}



static size_t HashByStackHasher(const void* a_key, size_t a_keySize) CPPUTILS_NOEXCEPT
{
    (void)a_keySize;
    StackInvestBacktrace*const pStack = (StackInvestBacktrace*)a_key;
    return StackInvestHashOfTheStack(pStack);
}


static bool IsTheSameStack(const void* a_key1, size_t a_keySize1, const void* a_key2, size_t a_keySize2) CPPUTILS_NOEXCEPT
{
    CPPUTILS_STATIC_CAST(void, a_keySize1);
    CPPUTILS_STATIC_CAST(void, a_keySize2);
    const StackInvestBacktrace*const pStack1 = (const StackInvestBacktrace*)a_key1;
    const StackInvestBacktrace*const pStack2 = (const StackInvestBacktrace*)a_key2;
    return StackInvestIsTheSameStack(pStack1,pStack2);
}


static bool StoreStackHashKey(TypeCinternalAllocator a_allocator, void** a_pKeyStore, size_t* a_pKeySizeStore, const void* a_key, size_t a_keySize) CPPUTILS_NOEXCEPT
{
    CPPUTILS_STATIC_CAST(void, a_allocator);
    CPPUTILS_STATIC_CAST(void, a_keySize);
    CPPUTILS_STATIC_CAST(void, a_pKeySizeStore);
    *a_pKeyStore = CPPUTILS_CONST_CAST(void*,a_key);
    return true;
}


static void UnstoreStackHashKey(TypeCinternalDeallocator a_deallocator, void* a_key, size_t a_keySize) CPPUTILS_NOEXCEPT
{
    CPPUTILS_STATIC_CAST(void, a_deallocator);
    CPPUTILS_STATIC_CAST(void, a_key);
    CPPUTILS_STATIC_CAST(void, a_keySize);
}


CPPUTILS_CODE_INITIALIZER(MemoryHandlerInit){
    s_nInitializationTimeNotPassed = 1;
    s_initTimeSec = time(&s_initTimeSec);

    s_pMutexForHashes = new MemLeakAnMutex();

    s_hashByAddress = CInternalDLLHashCreateExSmlInt(MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS,&MemoryHandlerCLibMalloc,&MemoryHandlerCLibFree);
    if(!s_hashByAddress){
        delete s_pMutexForHashes;
        exit(1);
    }

    s_hashByStack = CInternalDLLHashCreateExAny(MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS,&HashByStackHasher,&IsTheSameStack,
                                                &StoreStackHashKey,&UnstoreStackHashKey,&MemoryHandlerCLibMalloc,&MemoryHandlerCLibFree);
    if(!s_hashByStack){
        CInternalDLLHashDestroy(s_hashByAddress);
        delete s_pMutexForHashes;
        exit(1);
    }

    MemoryHandlerSetMallocFnc(&MemoryLeakAnalyzeMalloc);
    MemoryHandlerSetCallocFnc(&MemoryLeakAnalyzeCalloc);
    MemoryHandlerSetReallocFnc(&MemoryLeakAnalyzeRealloc);
    MemoryHandlerSetFreeFnc(&MemoryLeakAnalyzeFree);
#ifdef MEM_HANDLER_MMAP_NEEDED
    MemoryHandlerSetMmapFnc(&MemoryLeakAnalyzeMmap);
#endif

    // todo: use atexit family to regiter cleaner
}

