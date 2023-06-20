//
// file:			crash_investigator_linux_simple_analyze.cpp
// path:			src/core/crash_investigator_linux_simple_analyze.cpp
// created on:		2023 Mar 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


#include <crash_investigator/analyze_leaking.h>
#include <crash_investigator/alloc_free.h>
#include <stack_investigator/investigator.h>
#include <cinternal/c_raii.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>


CPPUTILS_BEGIN_C

#ifdef _WIN32
#define ctime_s_t(_timep,_buffer,_numberOfElements)     ctime_s(_buffer,_numberOfElements,_timep)
#else
#define ctime_s_t(_timep,_buffer,_numberOfElements)     CPPUTILS_STATIC_CAST(void,_numberOfElements);ctime_r(_timep,_buffer)
#endif

#define MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS        131072
#define MEM_LEAK_ANALYZE_HASH_BY_STACK_BASKETS      8192
#define MEMORY_LEAK_ANALYZE_INIT_TIME_SEC_DEFAULT   8
#define MEMORY_LEAK_ANALYZE_MAX_ALLOC_DEFAULT       300



struct SMemoryLeakAnalyseItem{
    struct StackInvestBacktrace*    pStack;
    size_t                          countForThisStack;
    CinternalDLLHashItem_t          hashByStackIter;
};


static inline long int GetIntFromVariableInline(const char* a_cpcVariableName){
    char* pcEnvValue = CPPUTILS_NULL;
#ifdef _WIN32
    char  vcEnvVariable[1024];
    if(GetEnvironmentVariableA(a_cpcVariableName,vcEnvVariable,1023)>0){
        pcEnvValue = vcEnvVariable;
    }
    else{
        pcEnvValue = CPPUTILS_NULL;
    }
#else
    #ifdef secure_getenv_defined
    pcEnvValue = secure_getenv(a_cpcVariableName);
    #else
    pcEnvValue = getenv(a_cpcVariableName);
    #endif
#endif

    if(pcEnvValue){
        long int lnVal = strtol(pcEnvValue,CPPUTILS_NULL,10);
        printf("!!!! %s = %ld\n",a_cpcVariableName,lnVal);
        fflush(stdout);
        return strtol(pcEnvValue,CPPUTILS_NULL,10);
    }
    return -1;
}


static void HashByStackDeallocator(void* a_pData)
{
    struct SMemoryLeakAnalyseItem*const pItem = (struct SMemoryLeakAnalyseItem*)a_pData;
    struct StackInvestBacktrace*const pStack = pItem->pStack;
    StackInvestFreeBacktraceData(pStack);
    MemoryHandlerCLibFree(pItem);
}


static size_t HashByStackHasher(const void* a_key, size_t a_keySize) CPPUTILS_NOEXCEPT
{
    (void)a_keySize;
    struct StackInvestBacktrace*const pStack = (struct StackInvestBacktrace*)a_key;
    return StackInvestHashOfTheStack(pStack);
}


static bool IsTheSameStack(const void* a_key1, size_t a_keySize1, const void* a_key2, size_t a_keySize2) CPPUTILS_NOEXCEPT
{
    CPPUTILS_STATIC_CAST(void, a_keySize1);
    CPPUTILS_STATIC_CAST(void, a_keySize2);
    const struct StackInvestBacktrace*const pStack1 = (const struct StackInvestBacktrace*)a_key1;
    const struct StackInvestBacktrace*const pStack2 = (const struct StackInvestBacktrace*)a_key2;
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


CRASH_INVEST_EXPORT int CrashInvestAnalyzeLeakingInitialize(struct SCrInvAnalyzeLeakingData* a_pTables, int*CPPUTILS_ARG_NN a_pnIgnoreForThisThread,
                                                            const char* a_cpcStartTimeEnv, const char* a_cpcMaxForStackEnv)
{
    int nRet;
    long int deltaTimeToStartAnalyze;
    int maxForStack;

    nRet = cinternal_lw_recursive_mutex_create(&(a_pTables->mutexForHashes));
    if(nRet){return nRet;}

    a_pTables->hashByAddress = CInternalDLLHashCreateExSmlInt(MEM_LEAK_ANALYZE_HASH_BY_ADR_BASKETS,&MemoryHandlerCLibMalloc,&MemoryHandlerCLibFree);
    if(!(a_pTables->hashByAddress)){
        cinternal_lw_recursive_mutex_destroy(&(a_pTables->mutexForHashes));
        return 1;
    }

    a_pTables->hashByStack = CInternalDLLHashCreateExAny(MEM_LEAK_ANALYZE_HASH_BY_STACK_BASKETS,&HashByStackHasher,&IsTheSameStack,
                                                         &StoreStackHashKey,&UnstoreStackHashKey,&MemoryHandlerCLibMalloc,&MemoryHandlerCLibFree);
    if(!(a_pTables->hashByStack)){
        CInternalDLLHashDestroy(a_pTables->hashByAddress);
        cinternal_lw_recursive_mutex_destroy(&(a_pTables->mutexForHashes));
        return 1;
    }

    a_pTables->pnIgnoreForThisThread = a_pnIgnoreForThisThread;
    *a_pnIgnoreForThisThread = 0;

    a_pTables->bInitializationTimeNotPassed = true;

    a_pTables->initTimeSec = time(&(a_pTables->initTimeSec));


    deltaTimeToStartAnalyze = GetIntFromVariableInline(a_cpcStartTimeEnv);
    if(deltaTimeToStartAnalyze<0){
        a_pTables->deltaTimeToStartAnalyze = MEMORY_LEAK_ANALYZE_INIT_TIME_SEC_DEFAULT;
    }
    else{
        a_pTables->deltaTimeToStartAnalyze = CPPUTILS_STATIC_CAST(time_t,deltaTimeToStartAnalyze);
    }


    maxForStack = GetIntFromVariableInline(a_cpcMaxForStackEnv);
    if(maxForStack<0){
        a_pTables->countForThisStackMax = MEMORY_LEAK_ANALYZE_MAX_ALLOC_DEFAULT;
    }
    else{
        a_pTables->countForThisStackMax = CPPUTILS_STATIC_CAST(size_t,maxForStack);
    }

    a_pTables->unMaxValue = 0;

    return 0;
}


CRASH_INVEST_EXPORT void CrashInvestAnalyzeLeakingClean(struct SCrInvAnalyzeLeakingData* a_pTables)
{
    CInternalDLLHashDestroy(a_pTables->hashByAddress);
    CInternalDLLHashDestroyEx(a_pTables->hashByStack,&HashByStackDeallocator);
    cinternal_lw_recursive_mutex_destroy(&(a_pTables->mutexForHashes));
}


static inline int CrashInvestAnalyzeLeakingAddAllocedItemNoLockInline(int a_goBackInTheStackCalc, void* a_ptr, struct SCrInvAnalyzeLeakingData* a_pTables){
    CinternalDLLHashItem_t hashIterAdr, hashIterStack;
    size_t unHashAdr, unHashStack;
    struct StackInvestBacktrace* pCurStack;
    struct SMemoryLeakAnalyseItem* pItem;

    pCurStack = StackInvestInitBacktraceDataForCurrentStack(a_goBackInTheStackCalc+1);
    if(!pCurStack){
        return 1;
    }

    hashIterStack = CInternalDLLHashFindEx(a_pTables->hashByStack,pCurStack,0,&unHashStack);
    if(hashIterStack){  // this stack is there
        char* pcTemp;
        char vcCurTimeStr[128];
        time_t currentTime;

        currentTime = time(&currentTime);
        ctime_s_t(&currentTime,vcCurTimeStr,127);
        pcTemp = strchr(vcCurTimeStr,'\n');
        StackInvestFreeBacktraceData(pCurStack);  //  no need to keep this stack data twce
        pItem = (struct SMemoryLeakAnalyseItem*)(hashIterStack->data);
        if((++(pItem->countForThisStack))>(a_pTables->countForThisStackMax)){
            struct StackInvestStackItem*const pStackItems = (struct StackInvestStackItem*)MemoryHandlerCLibMalloc(((size_t)(pItem->pStack->stackDeepness))*sizeof(struct StackInvestStackItem));
            if(pStackItems){
                if(!StackInvestConvertBacktraceToNames(pItem->pStack,pStackItems)){
                    int i=0;
                    for(;i<pItem->pStack->stackDeepness;++i){
                        if(pStackItems[i].line>=0){
                            fprintf(stderr,"bin: %s, %s(%d), fn:%s\n",pStackItems[i].binFile, pStackItems[i].sourceFile, pStackItems[i].line, pStackItems[i].funcName);
                        }
                        else{
                            fprintf(stderr,"%s\n",pStackItems[i].binFile);
                        }
                    }  //  for(;i<pItem->pStack->stackDeepness;++i){
                }  //  if(!StackInvestConvertBacktraceToNames(pItem->pStack,pStackItems)){
            }  //  if(pStackItems){
            fprintf(stderr,"  possible memory leak!!!!!\n");
            fflush(stderr);
            //StackInvestPrintTrace();
            exit(1);
        }
        else if(pItem->countForThisStack>(a_pTables->unMaxValue)){
            a_pTables->unMaxValue = pItem->countForThisStack;
            if(pcTemp){
                *pcTemp = 0;
                fprintf(stderr,"!!! %s",vcCurTimeStr);
            }
            else{
                fprintf(stderr,"!!!!!!!");
            }
            fprintf(stderr," new max =>  %d\n",(int)(a_pTables->unMaxValue));
            fflush(stderr);
        }
    }  //  if(hashIterStack){
    else{
        pItem = (struct SMemoryLeakAnalyseItem*)MemoryHandlerCLibMalloc(sizeof(struct SMemoryLeakAnalyseItem));
        if(!pItem){
            StackInvestFreeBacktraceData(pCurStack);
            MemoryHandlerCLibFree(a_ptr);
            return 1;
        }
        pItem->pStack = pCurStack;
        pItem->countForThisStack = 1;
        pItem->hashByStackIter = CInternalDLLHashAddDataWithKnownHash(a_pTables->hashByStack,pItem,pCurStack,0,unHashStack);
    }

    hashIterAdr = CInternalDLLHashFindEx(a_pTables->hashByAddress,a_ptr,0,&unHashAdr);
    assert(hashIterAdr==CPPUTILS_NULL);
    CInternalDLLHashAddDataWithKnownHash(a_pTables->hashByAddress,pItem,a_ptr,0,unHashAdr);

    return 0;
}


static inline void CrashInvestAnalyzeLeakingRemoveAllocedItemNoLockInline(void* a_ptr, struct SCrInvAnalyzeLeakingData* a_pTables){
    CinternalDLLHashItem_t hashIterAdr;
    size_t unHashAdr;

    hashIterAdr = CInternalDLLHashFindEx(a_pTables->hashByAddress,a_ptr,0,&unHashAdr);
    // assert(hashIterAdr);
    // `assert` is not ok, because of initialization time, some memories are not here  // this is not correct
    // some memories are not here because when locking done with s_nIgnoreForThisThread, then it is not inserted
    if(hashIterAdr){
        struct SMemoryLeakAnalyseItem* pItem;

        pItem = (struct SMemoryLeakAnalyseItem*)(hashIterAdr->data);
        CInternalDLLHashRemoveDataEx(a_pTables->hashByAddress,hashIterAdr);
        assert((pItem->countForThisStack)>0);
        if((--(pItem->countForThisStack))==0){
            struct StackInvestBacktrace*const pStack = pItem->pStack;
            CInternalDLLHashRemoveDataEx(a_pTables->hashByStack,pItem->hashByStackIter);
            StackInvestFreeBacktraceData(pStack);
            MemoryHandlerCLibFree(pItem);
        }
    }  //  if(hashIterAdr){
}


CRASH_INVEST_EXPORT int  CrashInvestAnalyzeLeakingAddAllocedItem(int a_goBackInTheStackCalc, void* a_ptr, struct SCrInvAnalyzeLeakingData* a_pTables)
{
    int nRet;

    if((*(a_pTables->pnIgnoreForThisThread))>0){
        return 0;
    }
    else {
        ++(*(a_pTables->pnIgnoreForThisThread));
        if(a_pTables->bInitializationTimeNotPassed){
            time_t currentTime;
            currentTime = time(&currentTime);
            if((currentTime - (a_pTables->initTimeSec))>(a_pTables->deltaTimeToStartAnalyze)){
                a_pTables->bInitializationTimeNotPassed = false;
            }
            else{
                --(*(a_pTables->pnIgnoreForThisThread));
                return 0;
            }
        }
    }  //  else of if((*(a_pTables->pnIgnoreForThisThread))>0){

    cinternal_lw_recursive_mutex_lock(&(a_pTables->mutexForHashes));
    nRet = CrashInvestAnalyzeLeakingAddAllocedItemNoLockInline(a_goBackInTheStackCalc+1,a_ptr,a_pTables);
    cinternal_lw_recursive_mutex_unlock(&(a_pTables->mutexForHashes));
    --(*(a_pTables->pnIgnoreForThisThread));
    return nRet;
}


CRASH_INVEST_EXPORT void CrashInvestAnalyzeLeakingRemoveAllocedItem(void* a_ptr, struct SCrInvAnalyzeLeakingData* a_pTables)
{
    ++(*(a_pTables->pnIgnoreForThisThread));
    cinternal_lw_recursive_mutex_lock(&(a_pTables->mutexForHashes));
    CrashInvestAnalyzeLeakingRemoveAllocedItemNoLockInline(a_ptr,a_pTables);
    cinternal_lw_recursive_mutex_unlock(&(a_pTables->mutexForHashes));
    --(*(a_pTables->pnIgnoreForThisThread));
}


CPPUTILS_END_C
