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
#include <crash_investigator/analyze_leaking.h>
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


CPPUTILS_BEGIN_C


static __thread int s_nIgnoreForThisThread = 0;
static struct SCrInvAnalyzeLeakingData   s_analyzeData;


static void* MemoryLeakAnalyzeMalloc(size_t a_size)
{
    void*const pRet = MemoryHandlerCLibMalloc(a_size);
    CrashInvestAnalyzeLeakingAddAllocedItem(1,pRet,&s_analyzeData);
    return pRet;
}


static void* MemoryLeakAnalyzeCalloc(size_t a_nmemb, size_t a_size)
{
    void*const pRet = MemoryHandlerCLibCalloc(a_nmemb,a_size);
    CrashInvestAnalyzeLeakingAddAllocedItem(1,pRet,&s_analyzeData);
    return pRet;
}


static void* MemoryLeakAnalyzeRealloc(void* a_ptr, size_t a_size)
{
    void*const pRet = MemoryHandlerCLibRealloc(a_ptr,a_size);
    if(a_size){
        if(pRet!=a_ptr){
            CrashInvestAnalyzeLeakingRemoveAllocedItem(a_ptr,&s_analyzeData);
            CrashInvestAnalyzeLeakingAddAllocedItem(1,pRet,&s_analyzeData);
        }
    }
    else{
        CrashInvestAnalyzeLeakingRemoveAllocedItem(a_ptr,&s_analyzeData);
    }

    return pRet;
}


static void MemoryLeakAnalyzeFree(void* a_ptr)
{
    CrashInvestAnalyzeLeakingRemoveAllocedItem(a_ptr,&s_analyzeData);
    MemoryHandlerCLibFree(a_ptr);
}


#ifndef _WIN32


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



CPPUTILS_DLL_PRIVATE void* STACK_INVEST_RW_MUTEX_CREATE_function(void){
    return (void*)1;
}


CPPUTILS_DLL_PRIVATE void STACK_INVEST_RW_MUTEX_DESTROY_function(void* a_pRwMutex){
    CPPUTILS_STATIC_CAST(void,a_pRwMutex);
}


CPPUTILS_DLL_PRIVATE void STACK_INVEST_RW_MUTEX_RD_LOCK_function(void* a_pRwMutex){
    CPPUTILS_STATIC_CAST(void,a_pRwMutex);
}


CPPUTILS_DLL_PRIVATE void STACK_INVEST_RW_MUTEX_WR_LOCK_function(void* a_pRwMutex){
    CPPUTILS_STATIC_CAST(void,a_pRwMutex);
}


CPPUTILS_DLL_PRIVATE void STACK_INVEST_RW_MUTEX_RD_UNLOCK_function(void* a_pRwMutex){
    CPPUTILS_STATIC_CAST(void,a_pRwMutex);
}


CPPUTILS_DLL_PRIVATE void STACK_INVEST_RW_MUTEX_WR_UNLOCK_function(void* a_pRwMutex){
    CPPUTILS_STATIC_CAST(void,a_pRwMutex);
}


#endif  //  #ifndef _WIN32


static void crash_investigator_linux_simple_analyze_clean(void){

    ++s_nIgnoreForThisThread;
    MemoryHandlerSetMallocFnc(&MemoryHandlerCLibMalloc);
    MemoryHandlerSetCallocFnc(&MemoryHandlerCLibCalloc);
    MemoryHandlerSetReallocFnc(&MemoryHandlerCLibRealloc);
    MemoryHandlerSetFreeFnc(&MemoryHandlerCLibFree);
#ifdef MEM_HANDLER_MMAP_NEEDED
    //MemoryHandlerSetMmapFnc(&MemoryLeakAnalyzeMmap);
#endif

    CrashInvestAnalyzeLeakingClean(&s_analyzeData);
    --s_nIgnoreForThisThread;

}


CPPUTILS_C_CODE_INITIALIZER(crash_investigator_linux_simple_analyze_init){

    if(CrashInvestAnalyzeLeakingInitialize(&s_analyzeData,&s_nIgnoreForThisThread,"MEMORY_LEAK_ANALYZE_INIT_TIME_SEC_DEFAULT","MEMORY_LEAK_ANALYZE_MAX_ALLOC_DEFAULT")){
        exit(1);
    }

    MemoryHandlerSetMallocFnc(&MemoryLeakAnalyzeMalloc);
    MemoryHandlerSetCallocFnc(&MemoryLeakAnalyzeCalloc);
    MemoryHandlerSetReallocFnc(&MemoryLeakAnalyzeRealloc);
    MemoryHandlerSetFreeFnc(&MemoryLeakAnalyzeFree);
#ifdef MEM_HANDLER_MMAP_NEEDED
    MemoryHandlerSetMmapFnc(&MemoryLeakAnalyzeMmap);
#endif

    atexit(&crash_investigator_linux_simple_analyze_clean);
}

CPPUTILS_END_C
