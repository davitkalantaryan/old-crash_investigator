//
// file:		crash_investigator_analyze_leak_only_new_delete.cpp
// path:		src/core/crash_investigator.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#ifdef use_crash_investigator_analyze_leak_only_new_delete

#include <monitor/core/monitor_internal_header.h>
#include <qtutils/core/logger.hpp>
#include <cpputils/hash/hash.hpp>
#include <cpputils/inscopecleaner.hpp>
#include <new>
#include <mutex>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#ifdef _WIN32
#include <crtdbg.h>
#include <WS2tcpip.h>
#include <WS2tcpip.h>
#include <Windows.h>
#else
#endif

extern thread_local bool s_bIgnoreThisStack;
thread_local bool s_bIgnoreThisStack = false;
extern thread_local bool s_bOperatorDeleteCalled;
thread_local bool s_bOperatorDeleteCalled = false;
static size_t s_unMaxNumberOfAllocInTheStack = 200;

static void* AllocMem(size_t a_size, int a_goBackInTheStackCalc);
static void  FreeMem(void* a_ptr, int a_goBackInTheStackCalc);

void* operator new  ( std::size_t a_count ){
        return AllocMem(a_count,1);
}
void* operator new []  ( std::size_t a_count ){
        return AllocMem(a_count,1);
}
void* operator new  ( std::size_t a_count, const std::nothrow_t& ) CPPUTILS_NODISCARD {
        return AllocMem(a_count,1);
}
void* operator new []  ( std::size_t a_count, const std::nothrow_t& ) CPPUTILS_NODISCARD{
        return AllocMem(a_count,1);
}

/*////////////////////////////////////////////////////////////////////////////////////////////*/

#ifdef CPPUTILS_CPP_17_DEFINED
void* operator new  ( std::size_t a_count, std::align_val_t a_al ){
        if(static_cast<std::size_t>(a_al)>a_count){a_count=static_cast<std::size_t>(a_al);}
        return AllocMem(a_count,1);
}
void* operator new []  ( std::size_t a_count, std::align_val_t a_al ){
    if(static_cast<std::size_t>(a_al)>a_count){a_count=static_cast<std::size_t>(a_al);}
    return AllocMem(a_count,1);
}
void* operator new  ( std::size_t a_count, std::align_val_t a_al, const std::nothrow_t& ) CPPUTILS_NODISCARD{
    if(static_cast<std::size_t>(a_al)>a_count){a_count=static_cast<std::size_t>(a_al);}
    return AllocMem(a_count,1);
}
void* operator new []  ( std::size_t a_count, std::align_val_t a_al, const std::nothrow_t& ) CPPUTILS_NODISCARD{
    if(static_cast<std::size_t>(a_al)>a_count){a_count=static_cast<std::size_t>(a_al);}
    return AllocMem(a_count,1);
}
#endif  // #ifdef CPPUTILS_CPP_17_DEFINED

void operator delete  ( void* a_ptr ) CPPUTILS_NOEXCEPT{
        FreeMem(a_ptr,1);
}
void operator delete [] ( void* a_ptr ) CPPUTILS_NOEXCEPT{
        FreeMem(a_ptr,1);
}
void operator delete  ( void* a_ptr, const std::nothrow_t& ) CPPUTILS_NOEXCEPT{
    FreeMem(a_ptr,1);
}
void operator delete[]( void* a_ptr, const std::nothrow_t& ) CPPUTILS_NOEXCEPT{
    FreeMem(a_ptr,1);
}

#ifdef CPPUTILS_CPP_14_DEFINED
void operator delete  ( void* a_ptr, std::size_t ) CPPUTILS_NOEXCEPT{
        FreeMem(a_ptr,1);
}
void operator delete [] ( void* a_ptr, std::size_t ) CPPUTILS_NOEXCEPT{
        FreeMem(a_ptr,1);
}
#endif  // #ifdef CPPUTILS_CPP_17_DEFINED


#ifdef CPPUTILS_CPP_17_DEFINED

void operator delete  ( void* a_ptr, std::align_val_t  ) CPPUTILS_NOEXCEPT{
        FreeMem(a_ptr,1);
}
void operator delete[]( void* a_ptr, std::align_val_t ) CPPUTILS_NOEXCEPT{
        FreeMem(a_ptr,1);
}

#endif  // #ifdef CPPUTILS_CPP_17_DEFINED



struct Backtrace{
    void** ppBuffer;
    int    stackDeepness;
    int    reserved01;
};

static Backtrace* InitBacktraceDataForCurrentStack(int a_goBackInTheStackCalc);
static void FreeBacktraceData(Backtrace* a_data);
static Backtrace* CloneBackTrace(const Backtrace* a_btr);
typedef Backtrace* BacktracePtr;
typedef void* VoidPtr;

struct BtVoidPtr{
    size_t operator()(const VoidPtr& a_mem) const {return (size_t)a_mem;}
};

struct BtHash{
    size_t operator()(const BacktracePtr& a_stack) const {
        size_t cunRet(0);
        size_t unMult(1);
        for (int i(0); i < a_stack->stackDeepness; ++i, unMult*=1000) {
            cunRet += ((size_t)a_stack->ppBuffer[i]) * unMult;
        }
        return cunRet;
    }
};


struct BackTrcEql {
    bool operator()(const BacktracePtr& a_lhs, const BacktracePtr& a_rhs) const {
        return (a_lhs->stackDeepness > 0) && (a_lhs->stackDeepness == a_rhs->stackDeepness) &&
                (memcmp(a_lhs->ppBuffer, a_rhs->ppBuffer, CPPUTILS_STATIC_CAST(size_t, a_lhs->stackDeepness) * sizeof(void*)) == 0);
    }
};

//typedef ::std::unordered_map<void*,Backtrace*,BtVoidPtr> HashMem;
//typedef ::std::unordered_map<Backtrace*,size_t,BtHash,BackTrcEql> HashStack;

CPPUTILS_DLL_PRIVATE void  free_default(void* a_ptr){HeapFree(GetProcessHeap(), 0, a_ptr);}
CPPUTILS_DLL_PRIVATE void* malloc_default(size_t a_count) { return HeapAlloc(GetProcessHeap(), 0, CPPUTILS_STATIC_CAST(SIZE_T, a_count));}
CPPUTILS_DLL_PRIVATE void* realloc_default(void* a_ptr, size_t a_count) { return HeapReAlloc(GetProcessHeap(), 0, a_ptr, CPPUTILS_STATIC_CAST(SIZE_T, a_count));}
CPPUTILS_DLL_PRIVATE void* calloc_default(size_t a_nmemb, size_t a_size) {
    const size_t unCount = a_nmemb * a_size;
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CPPUTILS_STATIC_CAST(SIZE_T, unCount));
}

#ifdef _WIN32
typedef ::cpputils::hash::Hash<void*,Backtrace*,BtVoidPtr,::std::equal_to<void*>,512,malloc_default,calloc_default,realloc_default,free_default> HashMem;
typedef ::cpputils::hash::Hash<Backtrace*,size_t,BtHash,BackTrcEql,512,malloc_default,calloc_default,realloc_default,free_default> HashStack;
#else
typedef ::cpputils::hash::Hash<void*,Backtrace*,BtVoidPtr> HashMem;
typedef ::cpputils::hash::Hash<Backtrace*,size_t,BtHash,BackTrcEql> HashStack;
#endif

static thread_local int s_isOngoing = 0;
static bool s_exitOngoing = false;

#if defined(_MSC_VER) && defined(_DEBUG)
static _CRT_ALLOC_HOOK s_initialAllocHook = CPPUTILS_NULL;
static int __CRTDECL CrashInvestMemHook(int, void*, size_t, int, long, unsigned char const*, int);
#endif

class IntHandler{
public:
    IntHandler(){++s_isOngoing;}
    ~IntHandler(){--s_isOngoing;}
};

class MemAnalyzeData{
public:
    MemAnalyzeData():m_pStack(nullptr),m_pMem(nullptr){
        IntHandler aHnd;
        m_pStack = new HashStack();
        m_pMem = new HashMem();
#if defined(_MSC_VER) && defined(_DEBUG)
        s_initialAllocHook = _CrtSetAllocHook(&CrashInvestMemHook);
#endif
    }
    ~MemAnalyzeData(){
        s_exitOngoing = true;
        IntHandler aHnd;
#if defined(_MSC_VER) && defined(_DEBUG)
        _CrtSetAllocHook(s_initialAllocHook);
#endif
        delete m_pMem;
        delete m_pStack;
        m_pMem = nullptr;
        m_pStack = nullptr;
    }
public:
    ::std::mutex    m_mutex;
    HashStack*      m_pStack;
    HashMem*      m_pMem;
}static s_memData;


static size_t s_unMaxInTheStack = 0;

static inline void TakeStackOut(const HashMem::iterator& a_iterMem)
{
    HashStack::iterator iterStack = s_memData.m_pStack->find(a_iterMem->second);
    assert (iterStack != s_memData.m_pStack->end());

    if(iterStack->second==1){
        Backtrace* pBacktrace2 = iterStack->first;
        s_memData.m_pStack->erase(iterStack);
        FreeBacktraceData(pBacktrace2);
    }
    else{
        --(iterStack->second);
    }
}


static void* AllocMem(size_t a_size, int a_goBackInTheStackCalc)
{
    size_t unHashStack;
    size_t unHashMem;

    //++s_nOperatorDeleteCalled;
    //::cpputils::InScopeCleaner aCleaner([](void*){--s_nOperatorDeleteCalled;});

    if(s_exitOngoing || (s_isOngoing>0) || s_bIgnoreThisStack){
        return :: malloc(a_size);
    }
    IntHandler aHndl;

    void* pRet = :: malloc(a_size);
    if(!pRet){return pRet;}

    HashMem::iterator iterMem;
    HashStack::iterator iterStack;
    Backtrace* pBacktrace = InitBacktraceDataForCurrentStack(++a_goBackInTheStackCalc);
	if(!pBacktrace){
		return pRet;
	}


    {
        ::std::unique_lock<std::mutex> aGuard(s_memData.m_mutex);


        iterMem = s_memData.m_pMem->find(pRet,&unHashMem);

#if !defined(_MSC_VER) || defined(_DEBUG)
        assert (iterMem == s_memData.m_pMem->end());
#else
        // we have this because this memory deallocated in the library level, and we are not able to monitor that memory
        if(iterMem != s_memData.m_pMem->end()){
            TakeStackOut(iterMem);
            Backtrace* pBacktraceRem = iterMem->second;
            s_memData.m_pMem->RemoveEntryRaw(iterMem);
            FreeBacktraceData(pBacktraceRem);
        }
#endif

        //(*s_memData.m_pMem)[pRet]=pBacktrace;
        s_memData.m_pMem->AddEntryWithKnownHashC(::std::pair<void*,Backtrace*>(pRet,pBacktrace),unHashMem);


        iterStack = s_memData.m_pStack->find(pBacktrace,&unHashStack);
        if (iterStack == s_memData.m_pStack->end()) {
            Backtrace* pBacktrace2 = CloneBackTrace(pBacktrace);
            //(*s_memData.m_pStack)[pBacktrace2]=1;
            s_memData.m_pStack->AddEntryWithKnownHashC(::std::pair<Backtrace*,size_t>(pBacktrace2,size_t(1)),unHashStack);
        }
        else{

            ++(iterStack->second);
            // todo: delete statistic
            if(iterStack->second>s_unMaxInTheStack){
                s_unMaxInTheStack = iterStack->second;
                QtUtilsDebug()<<"!!!! new max alloc in the stack: "<<s_unMaxInTheStack;
            }

            if(iterStack->second>s_unMaxNumberOfAllocInTheStack){
                aGuard.unlock();
                QtUtilsCritical()<<"Possible memory leak!";
                exit(1);
            }
        }

    }

    static_cast<void>(a_goBackInTheStackCalc);
    return pRet;
}


static void  FreeMemAnalyze(void* a_ptr, int a_goBackInTheStackCalc)
{
    if(s_exitOngoing){return;}
    IntHandler aHndl;

    HashMem::iterator iterMem;

    {
        ::std::unique_lock<std::mutex> aGuard(s_memData.m_mutex,::std::defer_lock);
        if(s_isOngoing<2){
            aGuard.lock();
        }

        iterMem = s_memData.m_pMem->find(a_ptr);
        // this (below line) can happen when we have early allocation, or a_ptr=null
        if(iterMem == s_memData.m_pMem->end()){return;}

        TakeStackOut(iterMem);
        Backtrace* pBacktraceRem = iterMem->second;
        s_memData.m_pMem->RemoveEntryRaw(iterMem);
        FreeBacktraceData(pBacktraceRem);
    }

    static_cast<void>(a_goBackInTheStackCalc);
}


static void  FreeMem(void* a_ptr, int a_goBackInTheStackCalc)
{
    FreeMemAnalyze(a_ptr,++a_goBackInTheStackCalc);

    {
        s_bOperatorDeleteCalled=true;
        ::cpputils::InScopeCleaner aCleaner([](void*){s_bOperatorDeleteCalled=false;});
        :: free(a_ptr);
    }

}


static Backtrace* CloneBackTrace(const Backtrace* a_btr)
{
    Backtrace* pReturn = static_cast<Backtrace*>(malloc_default(sizeof(Backtrace)));
    if(!pReturn){return pReturn;}
    pReturn->stackDeepness = a_btr->stackDeepness;
    pReturn->reserved01 = a_btr->reserved01;
    pReturn->ppBuffer = static_cast<void**>(malloc_default(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void*)));
    if(!pReturn->ppBuffer){
        free_default(pReturn);
        return nullptr;
    }

    memcpy(pReturn->ppBuffer, a_btr->ppBuffer, CPPUTILS_STATIC_CAST(size_t, a_btr->stackDeepness) * sizeof(void*));
    return pReturn;
}


#ifdef _WIN32

static void FreeBacktraceData(Backtrace* a_data)
{
    if (a_data) {
        free_default(a_data->ppBuffer);
        free_default(a_data);
    }
}

static Backtrace* InitBacktraceDataForCurrentStack(int a_goBackInTheStackCalc)
{
    ++a_goBackInTheStackCalc;
    void** ppBuffer = static_cast<void**>(alloca(static_cast<size_t>(64) * sizeof(void*)));
    WORD countOfStacks = CaptureStackBackTrace(static_cast<DWORD>(a_goBackInTheStackCalc), static_cast<DWORD>(64 - a_goBackInTheStackCalc), ppBuffer, CPPUTILS_NULL);

    if (countOfStacks < 1) { 
		return CPPUTILS_NULL; 
	}

    Backtrace* pReturn = static_cast<Backtrace*>(malloc_default(sizeof(Backtrace)));
    if(!pReturn){return CPPUTILS_NULL;}

    pReturn->stackDeepness = static_cast<int>(countOfStacks);

    pReturn->ppBuffer = static_cast<void**>(malloc_default(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void*)));
    if (!(pReturn->ppBuffer)) { 
		FreeBacktraceData(pReturn);
		return CPPUTILS_NULL;
	}

    memcpy(pReturn->ppBuffer, ppBuffer, static_cast<size_t>(pReturn->stackDeepness) * sizeof(void*));

    return pReturn;
}
#else
#endif

#if defined(_MSC_VER) && defined(_DEBUG)

static int __CRTDECL CrashInvestMemHook(int a_allocType, void* a_ptr, size_t, int, long, unsigned char const*, int)
{
    if(s_bOperatorDeleteCalled){return TRUE;}

    switch(a_allocType){
    case _HOOK_FREE: case _HOOK_REALLOC:
        FreeMemAnalyze(a_ptr,1);
        break;
    default:
        break;
    }
    return TRUE;
}

#endif  // #if defined(_MSC_VER) && defined(_DEBUG)


#endif  //  #ifdef use_crash_investigator_analyze_leak_only_new_delete
