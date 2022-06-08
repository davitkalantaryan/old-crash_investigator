//
// file:		crash_investigator_analyze_leak_only_new_delete.cpp
// path:		src/core/crash_investigator.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#ifdef use_crash_investigator_analyze_leak_only_new_delete

#include <cpputils/internal_header.h>
#include <functional>
#include <assert.h>
#include <unordered_map>
#include <mutex>
#include <new>
#include <string>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#ifdef _WIN32
#include <WS2tcpip.h>
#include <Windows.h>
#include <crtdbg.h>
#include <DbgHelp.h>
#ifdef _MSC_VER
#pragma comment (lib,"Dbghelp.lib")
#endif
#else
#if !defined(__EMSCRIPTEN__)
#define CRASH_INVESTEXECINFO_DEFINED
#endif
#ifdef CRASH_INVESTEXECINFO_DEFINED
#include <execinfo.h>
#endif
#include <unistd.h>
#if defined(__linux__) || defined(__linux)
#define CRASH_INVEST_PRCTL_DEFINED
#endif
#ifdef CRASH_INVEST_PRCTL_DEFINED
#include <sys/prctl.h>
#include <sys/wait.h>
#endif
#endif

#define CRASH_INVEST_SYMBOLS_COUNT_MAX 256
#define CRASH_INVEST_INIT_TIME			0 // s

extern thread_local bool g_bIgnoreThisStack;
thread_local bool g_bIgnoreThisStack = false;
extern bool g_initOrExitOngoing;
bool g_initOrExitOngoing = false;
static thread_local int s_isOngoing = 0;
static thread_local bool s_bOperatorDeleteCalled = false;
static size_t s_unMaxNumberOfAllocInTheStack = 100;

static time_t s_init_time = 0;


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

struct Backtrace{
	void **ppBuffer;
	int stackDeepness;
	int reserved01;
};

struct StackItem{
    void*           address;
    ::std::string   dllName;
    ::std::string   funcName;
    ::std::string   sourceFileName;  // empty means unavailable
    int             line;      // <0 means unknown (no debug info available)
    int             reserved01;
};

static Backtrace *InitBacktraceDataForCurrentStack(int a_goBackInTheStackCalc);
static void FreeBacktraceData(Backtrace *a_data);
static Backtrace *CloneBackTrace(const Backtrace *a_btr);
typedef Backtrace *BacktracePtr;
typedef void *VoidPtr;

struct BtVoidPtr{
	size_t operator()(const VoidPtr &a_mem) const { 
		return (size_t)a_mem; 
	}
};

struct BtHash{
	size_t operator()(const BacktracePtr &a_stack) const{
		size_t cunRet(0);
		size_t unMult(1);
		for (int i(0); i < a_stack->stackDeepness; ++i, unMult *= 1000)
		{
			cunRet += ((size_t)a_stack->ppBuffer[i]) * unMult;
		}
		return cunRet;
	}
};

struct BackTrcEql{
	bool operator()(const BacktracePtr &a_lhs, const BacktracePtr &a_rhs) const{
		return (a_lhs->stackDeepness > 0) && (a_lhs->stackDeepness == a_rhs->stackDeepness) &&
			   (memcmp(a_lhs->ppBuffer, a_rhs->ppBuffer, CPPUTILS_STATIC_CAST(size_t, a_lhs->stackDeepness) * sizeof(void *)) == 0);
	}
};


#ifdef _WIN32

static HANDLE s_currentProcess = CPPUTILS_NULL;

#else
#endif

#define free_c_lib_no_clbk      :: free
#define malloc_c_lib_no_clbk	:: malloc

template<class Key, class T, class Hash, class KeyEqual, class Allocator = std::allocator< std::pair<const Key, T> > >
struct NewHash : public ::std::unordered_map <Key, T, Hash, KeyEqual, Allocator> {
	using ::std::unordered_map<Key, T, Hash, KeyEqual, Allocator>::unordered_map;
	::std::unordered_map <Key, T, Hash, KeyEqual, Allocator>::iterator find(const Key& a_key, size_t* p = nullptr) {
		static_cast<void>(p); return ::std::unordered_map<Key, T, Hash, KeyEqual, Allocator>::find(a_key);
	}
	void AddEntryWithKnownHashC(const ::std::pair<const Key, T>& a_pair, size_t) {
		::std::unordered_map<Key, T, Hash, KeyEqual, Allocator>::insert(a_pair);
	}
	void RemoveEntryRaw(const ::std::unordered_map <Key, T, Hash, KeyEqual, Allocator>::iterator& a_iter) {
		::std::unordered_map<Key, T, Hash, KeyEqual, Allocator>::erase(a_iter);
	}
};

template <typename DataType>
struct SAllocator : public ::std::allocator<DataType> {
	DataType* allocate(size_t a_n, const void* a_hint = 0) {
		static_cast<void>(a_hint); return malloc_c_lib_no_clbk(a_n * sizeof(DataType));
	}
	void deallocate(DataType* a_p, std::size_t a_n) {
		static_cast<void>(a_n); free_c_lib_no_clbk(a_p);
	}
};

typedef NewHash<void*, Backtrace*, BtVoidPtr, ::std::equal_to<void*>, SAllocator<std::pair<void* const, Backtrace*> > > HashMem;
typedef NewHash<Backtrace*, size_t, BtHash, BackTrcEql, SAllocator<std::pair<Backtrace* const, size_t> > > HashStack;

#if defined(_MSC_VER) && defined(_DEBUG)
static _CRT_ALLOC_HOOK s_initialAllocHook = CPPUTILS_NULL;
static int __CRTDECL CrashInvestMemHook(int, void *, size_t, int, long, unsigned char const *, int);
#endif

class IntHandler
{
  public:
	IntHandler() { ++s_isOngoing; }
	~IntHandler() { --s_isOngoing; g_initOrExitOngoing = false; }
};

static HashStack *s_pStack = nullptr;
static HashMem *s_pMem = nullptr;
static ::std::mutex* s_pMutex = nullptr;
static std::once_flag s_call_once_flag;

class MemAnalyzeData
{
  public:
	MemAnalyzeData(){
		this->InitRaw();
	}
	~MemAnalyzeData(){
		g_initOrExitOngoing = true;

#if defined(_MSC_VER) && defined(_DEBUG)
		_CrtSetAllocHook(s_initialAllocHook);
#endif
		delete s_pMem; s_pMem=nullptr;
		delete s_pStack; s_pStack = nullptr;
		delete s_pMutex; s_pMutex = nullptr;
	}

	void Init() {
		if (!s_pMutex){
			std::call_once(s_call_once_flag, [this]() {
				InitRaw();
			});
		} 
	}
	
private:
	void InitRaw(void){
		g_initOrExitOngoing = true;
		IntHandler aHnd;
		s_init_time = time(&s_init_time);

		s_pMutex = new ::std::mutex();
		s_pStack = new HashStack(8192);
		s_pMem = new HashMem(131072);
#if defined(_MSC_VER) && defined(_DEBUG)
		s_initialAllocHook = _CrtSetAllocHook(&CrashInvestMemHook);
#endif

#ifdef _WIN32
		s_currentProcess = GetCurrentProcess();
		if (!SymInitialize(s_currentProcess, CPPUTILS_NULL, TRUE)) {
			// SymInitialize failed
			DWORD error = GetLastError();
			fprintf(stderr, "SymInitialize returned error : %d\n", static_cast<int>(error));
			return;
		}
#endif

	}

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


static void ConvertBacktraceToNames(const Backtrace* a_data, ::std::vector< StackItem>*  a_pStack);
static void print_trace(void);

static void *AllocMem(size_t a_size, int a_goBackInTheStackCalc)
{
	size_t unHashStack;
	size_t unHashMem;

	if (g_initOrExitOngoing || (s_isOngoing > 0) || g_bIgnoreThisStack){
        return malloc_c_lib_no_clbk(a_size);
	}
	IntHandler aHndl;
	s_memData.Init();

    void *pRet = malloc_c_lib_no_clbk(a_size);
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
		::std::unique_lock<std::mutex> aGuard(*s_pMutex);

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
					::std::vector<StackItem> stackTracce;
					ConvertBacktraceToNames(pBacktrace,&stackTracce);
					const size_t cunNumberOfFrames(stackTracce.size());
					const StackItem* pFrames = stackTracce.data();
					fprintf(stderr,"!!!!!! possible place of memory leak\n");
					for(size_t i(0); i<cunNumberOfFrames;++i){
						fprintf(stderr,"\t%p, bin:\"%s\", fnc:\"%s\", src:\"%s\", ln:%d\n",
								pFrames[i].address,pFrames[i].dllName.c_str(),pFrames[i].funcName.c_str(),
								pFrames[i].sourceFileName.c_str(), pFrames[i].line);
					}
					print_trace();
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
	if (g_initOrExitOngoing){
		return;
	}
	IntHandler aHndl;

	HashMem::iterator iterMem;

	{
		::std::unique_lock<std::mutex> aGuard(*s_pMutex, ::std::defer_lock);
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
        free_c_lib_no_clbk(a_ptr);
		s_bOperatorDeleteCalled = false;
	}
}


static Backtrace* CloneBackTrace(const Backtrace* a_btr)
{
	if (a_btr){
		Backtrace *pReturn = static_cast<Backtrace *>(malloc_c_lib_no_clbk(sizeof(Backtrace)));
		if (!pReturn){
			return pReturn;
		}
		pReturn->stackDeepness = a_btr->stackDeepness;
		pReturn->reserved01 = a_btr->reserved01;
		pReturn->ppBuffer = static_cast<void **>(malloc_c_lib_no_clbk(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *)));
		if (!pReturn->ppBuffer){
			free_c_lib_no_clbk(pReturn);
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
		free_c_lib_no_clbk(a_data->ppBuffer);
		free_c_lib_no_clbk(a_data);
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

	Backtrace *pReturn = static_cast<Backtrace *>(malloc_c_lib_no_clbk(sizeof(Backtrace)));
	if (!pReturn)
	{
		return CPPUTILS_NULL;
	}

	pReturn->stackDeepness = static_cast<int>(countOfStacks);

	pReturn->ppBuffer = static_cast<void **>(malloc_c_lib_no_clbk(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *)));
	if (!(pReturn->ppBuffer))
	{
		FreeBacktraceData(pReturn);
		return CPPUTILS_NULL;
	}

	memcpy(pReturn->ppBuffer, ppBuffer, static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *));

	return pReturn;
}

#ifdef _WIN64
typedef DWORD64  DWORD_ci;
#else
typedef DWORD  DWORD_ci;
#endif


static void GetSymbolInfo(StackItem* a_pStackItem)
{
	// https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address
	const DWORD_ci  dwAddress = static_cast<DWORD_ci>(reinterpret_cast<size_t>(a_pStackItem->address));

	{
		DWORD64  dwDisplacement = 0;
		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		if (SymFromAddr(s_currentProcess, dwAddress, &dwDisplacement, pSymbol)) {
			a_pStackItem->funcName = pSymbol->Name;
		}
	}

	
	
	{
		DWORD  dwDisplacement;
		IMAGEHLP_LINE64 line;

		SymSetOptions(SYMOPT_LOAD_LINES);

		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

		if (SymGetLineFromAddr64(s_currentProcess, dwAddress, &dwDisplacement, &line)){
			if (line.FileName) {
				a_pStackItem->sourceFileName = line.FileName;
			}
			a_pStackItem->line = static_cast<int>(line.LineNumber);
		}
		else{
			a_pStackItem->line = -1;
		}
	}


	{
		IMAGEHLP_MODULE aModuleInfo;
		aModuleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

		if (SymGetModuleInfo(s_currentProcess, dwAddress, &aModuleInfo)) {
			//printf("ModuleName=\"%s\"\n", aModuleInfo.ModuleName);
			//printf("ImageName=\"%s\"\n", aModuleInfo.ImageName);
			//printf("LoadedImageName=\"%s\"\n", aModuleInfo.LoadedImageName);
			a_pStackItem->dllName = aModuleInfo.ImageName;
		}
	}


}


static void ConvertBacktraceToNames(const Backtrace* a_data, ::std::vector< StackItem>*  a_pStack)
{
	StackItem* pStackItem;
	const size_t cunSynbols(a_data->stackDeepness);
	a_pStack->resize(cunSynbols);

	for (size_t i(0); i < cunSynbols; ++i) {
		pStackItem = &(a_pStack->operator [](i));
		pStackItem->reserved01 = 0;
		pStackItem->address = a_data->ppBuffer[i];
		GetSymbolInfo(pStackItem);
	}
}


static void print_trace(void){}


#else


#ifdef CRASH_INVESTEXECINFO_DEFINED

static Backtrace* InitBacktraceDataForCurrentStack(int a_goBackInTheStackCalc)
{
	Backtrace *pReturn = static_cast<Backtrace *>(malloc_c_lib_no_clbk(sizeof(Backtrace)));
	if (!pReturn){
		return CPPUTILS_NULL;
	}

	const int cnMaxSymbolCount = CRASH_INVEST_SYMBOLS_COUNT_MAX + a_goBackInTheStackCalc;

	pReturn->reserved01 = 0;

	void **ppBuffer = static_cast<void **>(alloca(static_cast<size_t>(cnMaxSymbolCount) * sizeof(void *)));
	int nInitialDeepness = backtrace(ppBuffer, cnMaxSymbolCount);
	if (nInitialDeepness > a_goBackInTheStackCalc){
		pReturn->stackDeepness = nInitialDeepness - a_goBackInTheStackCalc;
		pReturn->ppBuffer = static_cast<void **>(malloc_c_lib_no_clbk(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *)));
		if (!(pReturn->ppBuffer)){
			FreeBacktraceData(pReturn);
			return CPPUTILS_NULL;
		}
		memcpy(pReturn->ppBuffer, &(ppBuffer[a_goBackInTheStackCalc]), static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *));
	}
	else{
		pReturn->stackDeepness = nInitialDeepness;
		pReturn->ppBuffer = static_cast<void **>(malloc_c_lib_no_clbk(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *)));
		if (!(pReturn->ppBuffer)){
			FreeBacktraceData(pReturn);
			return CPPUTILS_NULL;
		}
		memcpy(pReturn->ppBuffer, ppBuffer, static_cast<size_t>(pReturn->stackDeepness) * sizeof(void *));
	}

	return pReturn;
}



static void ConvertBacktraceToNames(const Backtrace* a_data, ::std::vector< StackItem>*  a_pStack)
{
    if(a_data){
        char** ppStrings = backtrace_symbols(a_data->ppBuffer,a_data->stackDeepness);
        if(!ppStrings){return;}

        StackItem* pStackItem;
        const size_t cunSynbols(a_data->stackDeepness);
        a_pStack->resize(cunSynbols);

        for(size_t i(0); i < cunSynbols; ++i){
            pStackItem = &(a_pStack->operator [](i));
            pStackItem->address = a_data->ppBuffer[i];
            pStackItem->dllName = ppStrings[i];
			pStackItem->reserved01 = 0;
			pStackItem->line = -1;
        }

		free_c_lib_no_clbk(ppStrings);
    }
}

#else

static Backtrace* InitBacktraceDataForCurrentStack(int){return nullptr;}
static void ConvertBacktraceToNames(const Backtrace*, ::std::vector< StackItem>* ){}

#endif

#ifdef CRASH_INVEST_PRCTL_DEFINED

static void print_trace(void) 
{
    char pid_buf[30];
    sprintf(pid_buf, "%d", getpid());
    char name_buf[512];
    name_buf[readlink("/proc/self/exe", name_buf, 511)]=0;
    prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
    int child_pid = fork();
    if (!child_pid) {
        dup2(2,1); // redirect output to stderr - edit: unnecessary?
        execl("/usr/bin/gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt", name_buf, pid_buf, NULL);
        abort(); /* If gdb failed to start */
    } else {
        waitpid(child_pid,NULL,0);
    }
}

#else

static void print_trace(void){}

#endif

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
