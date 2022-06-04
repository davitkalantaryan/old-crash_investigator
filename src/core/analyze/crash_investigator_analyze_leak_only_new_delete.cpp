//
// file:		crash_investigator_analyze_leak_only_new_delete.cpp
// path:		src/core/crash_investigator.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#ifdef use_crash_investigator_analyze_leak_only_new_delete

// #include <qtutils/core/logger.hpp>
#include <functional>
#include <assert.h>
#include <cpputils/hash/hash.hpp>
#include <mutex>
#include <new>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <WS2tcpip.h>
#include <Windows.h>
#include <crtdbg.h>
#else
#include <execinfo.h>
#endif

#define CRASH_INVEST_SYMBOLS_COUNT_MAX 256
#define CRASH_INVEST_INIT_TIME			0 // s

extern thread_local bool g_bIgnoreThisStack;
thread_local bool g_bIgnoreThisStack = false;
extern bool g_exitOngoing;
bool g_exitOngoing = false;
static thread_local int s_isOngoing = 0;
static thread_local bool s_bOperatorDeleteCalled = false;
static size_t s_unMaxNumberOfAllocInTheStack = 100;

static time_t s_init_time = 0;

class InScopeCleaner{
public:
	InScopeCleaner(const std::function<void(void)> &a_cleaner) : m_cleaner(a_cleaner){}
	~InScopeCleaner() {m_cleaner();}
private:
	const std::function<void(void)> m_cleaner;
};


static void* AllocMem(size_t a_size, int a_goBackInTheStackCalc);
static void  FreeMem(void *a_ptr, int a_goBackInTheStackCalc);

void *operator new(std::size_t a_count)
{
	return AllocMem(a_count, 1);
}
void *operator new[](std::size_t a_count)
{
	return AllocMem(a_count, 1);
}
void *operator new(std::size_t a_count, const std::nothrow_t &) CPPUTILS_NODISCARD
{
	return AllocMem(a_count, 1);
}
void *operator new[](std::size_t a_count, const std::nothrow_t &) CPPUTILS_NODISCARD
{
	return AllocMem(a_count, 1);
}

/*////////////////////////////////////////////////////////////////////////////////////////////*/

#ifdef CPPUTILS_CPP_17_DEFINED
void *operator new(std::size_t a_count, std::align_val_t a_al)
{
	if (static_cast<std::size_t>(a_al) > a_count)
	{
		a_count = static_cast<std::size_t>(a_al);
	}
	return AllocMem(a_count, 1);
}
void *operator new[](std::size_t a_count, std::align_val_t a_al)
{
	if (static_cast<std::size_t>(a_al) > a_count)
	{
		a_count = static_cast<std::size_t>(a_al);
	}
	return AllocMem(a_count, 1);
}
void *operator new(std::size_t a_count, std::align_val_t a_al, const std::nothrow_t &) CPPUTILS_NODISCARD
{
	if (static_cast<std::size_t>(a_al) > a_count)
	{
		a_count = static_cast<std::size_t>(a_al);
	}
	return AllocMem(a_count, 1);
}
void *operator new[](std::size_t a_count, std::align_val_t a_al, const std::nothrow_t &) CPPUTILS_NODISCARD
{
	if (static_cast<std::size_t>(a_al) > a_count)
	{
		a_count = static_cast<std::size_t>(a_al);
	}
	return AllocMem(a_count, 1);
}
#endif // #ifdef CPPUTILS_CPP_17_DEFINED

void operator delete(void *a_ptr)CPPUTILS_NOEXCEPT
{
	FreeMem(a_ptr, 1);
}
void operator delete[](void *a_ptr) CPPUTILS_NOEXCEPT
{
	FreeMem(a_ptr, 1);
}
void operator delete(void *a_ptr, const std::nothrow_t &)CPPUTILS_NOEXCEPT
{
	FreeMem(a_ptr, 1);
}
void operator delete[](void *a_ptr, const std::nothrow_t &) CPPUTILS_NOEXCEPT
{
	FreeMem(a_ptr, 1);
}

#ifdef CPPUTILS_CPP_14_DEFINED
void operator delete(void *a_ptr, std::size_t)CPPUTILS_NOEXCEPT
{
	FreeMem(a_ptr, 1);
}
void operator delete[](void *a_ptr, std::size_t) CPPUTILS_NOEXCEPT
{
	FreeMem(a_ptr, 1);
}
#endif // #ifdef CPPUTILS_CPP_17_DEFINED

#ifdef CPPUTILS_CPP_17_DEFINED

void operator delete(void *a_ptr, std::align_val_t)CPPUTILS_NOEXCEPT
{
	FreeMem(a_ptr, 1);
}
void operator delete[](void *a_ptr, std::align_val_t) CPPUTILS_NOEXCEPT
{
	FreeMem(a_ptr, 1);
}

#endif // #ifdef CPPUTILS_CPP_17_DEFINED

struct Backtrace
{
	void **ppBuffer;
	int stackDeepness;
	int reserved01;
};

static Backtrace *InitBacktraceDataForCurrentStack(int a_goBackInTheStackCalc);
static void FreeBacktraceData(Backtrace *a_data);
static Backtrace *CloneBackTrace(const Backtrace *a_btr);
typedef Backtrace *BacktracePtr;
typedef void *VoidPtr;

struct BtVoidPtr
{
	size_t operator()(const VoidPtr &a_mem) const { 
		return (size_t)a_mem; 
	}
};

struct BtHash
{
	size_t operator()(const BacktracePtr &a_stack) const
	{
		size_t cunRet(0);
		size_t unMult(1);
		for (int i(0); i < a_stack->stackDeepness; ++i, unMult *= 1000)
		{
			cunRet += ((size_t)a_stack->ppBuffer[i]) * unMult;
		}
		return cunRet;
	}
};

struct BackTrcEql
{
	bool operator()(const BacktracePtr &a_lhs, const BacktracePtr &a_rhs) const
	{
		return (a_lhs->stackDeepness > 0) && (a_lhs->stackDeepness == a_rhs->stackDeepness) &&
			   (memcmp(a_lhs->ppBuffer, a_rhs->ppBuffer, CPPUTILS_STATIC_CAST(size_t, a_lhs->stackDeepness) * sizeof(void *)) == 0);
	}
};


#ifdef _WIN32

CPPUTILS_DLL_PRIVATE void free_default(void *a_ptr)
{
	HeapFree(GetProcessHeap(), 0, a_ptr);
}
CPPUTILS_DLL_PRIVATE void *malloc_default(size_t a_count) { return HeapAlloc(GetProcessHeap(), 0, CPPUTILS_STATIC_CAST(SIZE_T, a_count)); }
CPPUTILS_DLL_PRIVATE void *realloc_default(void *a_ptr, size_t a_count) { return HeapReAlloc(GetProcessHeap(), 0, a_ptr, CPPUTILS_STATIC_CAST(SIZE_T, a_count)); }
CPPUTILS_DLL_PRIVATE void *calloc_default(size_t a_nmemb, size_t a_size)
{
	const size_t unCount = a_nmemb * a_size;
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CPPUTILS_STATIC_CAST(SIZE_T, unCount));
}

#else
#define free_default free
#define malloc_default malloc
#define realloc_default realloc
#define calloc_default calloc
#endif

typedef ::cpputils::hash::Hash<void *, Backtrace *, BtVoidPtr, ::std::equal_to<void *>, 512, malloc_default, calloc_default, realloc_default, free_default> HashMem;
typedef ::cpputils::hash::Hash<Backtrace *, size_t, BtHash, BackTrcEql, 512, malloc_default, calloc_default, realloc_default, free_default> HashStack;

#if defined(_MSC_VER) && defined(_DEBUG)
static _CRT_ALLOC_HOOK s_initialAllocHook = CPPUTILS_NULL;
static int __CRTDECL CrashInvestMemHook(int, void *, size_t, int, long, unsigned char const *, int);
#endif

class IntHandler
{
  public:
	IntHandler() { ++s_isOngoing; }
	~IntHandler() { --s_isOngoing; }
};

static HashStack *s_pStack = nullptr;
static HashMem *s_pMem = nullptr;
static ::std::mutex* s_pMutex = nullptr;
static std::once_flag s_call_once_flag;

class MemAnalyzeData
{
  public:
	MemAnalyzeData(){
		this->Init();
	}
	~MemAnalyzeData(){
		g_exitOngoing = true;
		IntHandler aHnd;
#if defined(_MSC_VER) && defined(_DEBUG)
		_CrtSetAllocHook(s_initialAllocHook);
#endif
		delete s_pMem; s_pMem=nullptr;
		delete s_pStack; s_pStack = nullptr;
		delete s_pMutex; s_pMutex = nullptr;
	}

	void Init() {
		if (!s_pMutex){
			IntHandler aHnd;

			std::call_once(s_call_once_flag, [this]() {
				
				s_init_time = time(&s_init_time);

				s_pMutex = new ::std::mutex();
				s_pStack = new HashStack(16384);
				s_pMem = new HashMem(131072);
#if defined(_MSC_VER) && defined(_DEBUG)
				s_initialAllocHook = _CrtSetAllocHook(&CrashInvestMemHook);
#endif
			});
		} 
	}

  public:
	::std::mutex m_mutex;
} static s_memData;


static inline void TakeStackOut(const HashMem::iterator &a_iterMem)
{
	HashStack::iterator iterStack = s_pStack->find(a_iterMem->second);	
#if !defined(_MSC_VER) || defined(_DEBUG)  // in the case of debug VS allows hooking
	assert(iterStack != s_pStack->end());
#else
	if(iterStack == s_pStack->end()){
		return;
	}
#endif

	if (iterStack->second == 1){
		Backtrace *pBacktrace2 = iterStack->first;
		s_pStack->erase(iterStack);
		FreeBacktraceData(pBacktrace2);
	}
	else{
		--(iterStack->second);
	}
}


static void *AllocMem(size_t a_size, int a_goBackInTheStackCalc)
{
	size_t unHashStack;
	size_t unHashMem;

	if (g_exitOngoing || (s_isOngoing > 0) || g_bIgnoreThisStack){
		return malloc_default(a_size);
	}
	IntHandler aHndl;
	s_memData.Init();

	void *pRet = malloc_default(a_size);
	if (!pRet){
		return pRet;
	}

	HashMem::iterator iterMem;
	HashStack::iterator iterStack;
	Backtrace *pBacktrace = InitBacktraceDataForCurrentStack(++a_goBackInTheStackCalc);
	if (!pBacktrace){
		return pRet;
	}

	{
		::std::unique_lock<std::mutex> aGuard(s_memData.m_mutex);

		iterMem = s_pMem->find(pRet, &unHashMem);

#if !defined(_MSC_VER) || defined(_DEBUG)  // in the case of debug VS allows hooking
		assert(iterMem == s_pMem->end());
#else
		// we have this because this memory deallocated in the library level, and we are not able to monitor that memory
		if (iterMem != s_pMem->end())
		{
			TakeStackOut(iterMem);
			Backtrace *pBacktraceRem = iterMem->second;
			s_pMem->RemoveEntryRaw(iterMem);
			FreeBacktraceData(pBacktraceRem);
		}
#endif

		s_pMem->AddEntryWithKnownHashC(::std::pair<void *, Backtrace *>(pRet, pBacktrace), unHashMem);

		iterStack = s_pStack->find(pBacktrace, &unHashStack);
		if (iterStack == s_pStack->end()){
			Backtrace *pBacktrace2 = CloneBackTrace(pBacktrace);
			s_pStack->AddEntryWithKnownHashC(::std::pair<Backtrace *, size_t>(pBacktrace2, size_t(1)), unHashStack);
		}
		else{
			time_t current_time;
			current_time = time(&current_time);

			++(iterStack->second);

			if (iterStack->second > s_unMaxNumberOfAllocInTheStack){

				if (current_time - s_init_time >= CRASH_INVEST_INIT_TIME){
					aGuard.unlock();
					// todo: print stack frames
					exit(1);
				}
			} // if (iterStack->second > s_unMaxNumberOfAllocInTheStack)
		}
	}

	static_cast<void>(a_goBackInTheStackCalc);
	return pRet;
}

static void FreeMemAnalyze(void *a_ptr, int a_goBackInTheStackCalc)
{
	s_memData.Init();

	if (g_exitOngoing){
		return;
	}
	IntHandler aHndl;

	HashMem::iterator iterMem;

	{
		::std::unique_lock<std::mutex> aGuard(s_memData.m_mutex, ::std::defer_lock);
		if (s_isOngoing < 2){
			aGuard.lock();
		}

		iterMem = s_pMem->find(a_ptr);
		// this (below line) can happen when we have early allocation, or a_ptr=null
		if (iterMem == s_pMem->end()){
			return;
		}
		
		TakeStackOut(iterMem);
		Backtrace *pBacktraceRem = iterMem->second;
		s_pMem->RemoveEntryRaw(iterMem);
		FreeBacktraceData(pBacktraceRem);
	}

	static_cast<void>(a_goBackInTheStackCalc);
}


static void FreeMem(void *a_ptr, int a_goBackInTheStackCalc)
{
	FreeMemAnalyze(a_ptr, ++a_goBackInTheStackCalc);

	{
		s_bOperatorDeleteCalled = true;
		InScopeCleaner aCleaner([]() { s_bOperatorDeleteCalled = false; });
		free_default(a_ptr);
	}
}


static Backtrace* CloneBackTrace(const Backtrace* a_btr)
{
	if (a_btr){
		Backtrace *pReturn = static_cast<Backtrace *>(malloc_default(sizeof(Backtrace)));
		if (!pReturn){
			return pReturn;
		}
		pReturn->stackDeepness = a_btr->stackDeepness;
		pReturn->reserved01 = a_btr->reserved01;
		pReturn->ppBuffer = static_cast<void **>(malloc_default(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *)));
		if (!pReturn->ppBuffer){
			free_default(pReturn);
			return nullptr;
		}

		memcpy(pReturn->ppBuffer, a_btr->ppBuffer, CPPUTILS_STATIC_CAST(size_t, a_btr->stackDeepness) * sizeof(void *));
		return pReturn;
	}
	return nullptr;
}


static void FreeBacktraceData(Backtrace *a_data)
{
	if (a_data){
		free_default(a_data->ppBuffer);
		free_default(a_data);
	}
}


#ifdef _WIN32

static Backtrace *InitBacktraceDataForCurrentStack(int a_goBackInTheStackCalc)
{
	++a_goBackInTheStackCalc;
	void **ppBuffer = static_cast<void **>(alloca(static_cast<size_t>(64) * sizeof(void *)));
	WORD countOfStacks = CaptureStackBackTrace(static_cast<DWORD>(a_goBackInTheStackCalc), static_cast<DWORD>(64 - a_goBackInTheStackCalc), ppBuffer, CPPUTILS_NULL);

	if (countOfStacks < 1)
	{
		return CPPUTILS_NULL;
	}

	Backtrace *pReturn = static_cast<Backtrace *>(malloc_default(sizeof(Backtrace)));
	if (!pReturn)
	{
		return CPPUTILS_NULL;
	}

	pReturn->stackDeepness = static_cast<int>(countOfStacks);

	pReturn->ppBuffer = static_cast<void **>(malloc_default(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *)));
	if (!(pReturn->ppBuffer))
	{
		FreeBacktraceData(pReturn);
		return CPPUTILS_NULL;
	}

	memcpy(pReturn->ppBuffer, ppBuffer, static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *));

	return pReturn;
}

#else

static Backtrace* InitBacktraceDataForCurrentStack(int a_goBackInTheStackCalc)
{
	Backtrace *pReturn = static_cast<Backtrace *>(malloc_default(sizeof(Backtrace)));
	if (!pReturn){
		return CPPUTILS_NULL;
	}

	const int cnMaxSymbolCount = CRASH_INVEST_SYMBOLS_COUNT_MAX + a_goBackInTheStackCalc;

	pReturn->reserved01 = 0;

	void **ppBuffer = static_cast<void **>(alloca(static_cast<size_t>(cnMaxSymbolCount) * sizeof(void *)));
	int nInitialDeepness = backtrace(ppBuffer, cnMaxSymbolCount);
	if (nInitialDeepness > a_goBackInTheStackCalc){
		pReturn->stackDeepness = nInitialDeepness - a_goBackInTheStackCalc;
		pReturn->ppBuffer = static_cast<void **>(malloc_default(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *)));
		if (!(pReturn->ppBuffer)){
			FreeBacktraceData(pReturn);
			return CPPUTILS_NULL;
		}
		memcpy(pReturn->ppBuffer, &(ppBuffer[a_goBackInTheStackCalc]), static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *));
	}
	else{
		pReturn->stackDeepness = nInitialDeepness;
		pReturn->ppBuffer = static_cast<void **>(malloc_default(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *)));
		if (!(pReturn->ppBuffer)){
			FreeBacktraceData(pReturn);
			return CPPUTILS_NULL;
		}
		memcpy(pReturn->ppBuffer, ppBuffer, static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *));
	}

	return pReturn;
}

#endif

#if defined(_MSC_VER) && defined(_DEBUG)

static int __CRTDECL CrashInvestMemHook(int a_allocType, void *a_ptr, size_t, int, long, unsigned char const *, int)
{
	if (s_bOperatorDeleteCalled){
		return TRUE;
	}

	switch (a_allocType)
	{
	case _HOOK_FREE:
	case _HOOK_REALLOC:
		FreeMemAnalyze(a_ptr, 1);
		break;
	default:
		break;
	}
	return TRUE;
}

#endif // #if defined(_MSC_VER) && defined(_DEBUG)

#endif //  #ifdef use_crash_investigator_analyze_leak_only_new_delete
