//
// file:		crash_investigator_malloc_free_hook_windows2.c
// path:		src/core/crash_investigator_malloc_free_hook_windows2.c
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 24
//

#if defined(_WIN32)

#include <crash_investigator/crash_investigator_internal_header.h>
#include <crash_investigator/crash_investigator_malloc_free_hook.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>



CPPUTILS_BEGIN_C

#ifdef _DEBUG
#define CRASH_INVEST_CRUN_LIB	"ucrtbased.dll"
#else
#define CRASH_INVEST_CRUN_LIB	"ucrtbase.dll"
#endif


CPPUTILS_DLL_PRIVATE void  CrInvAllocFree_free_default(void* a_ptr){
	HMODULE cranLib = LoadLibraryA(CRASH_INVEST_CRUN_LIB);
	if (cranLib) {
		TypeFree freePtr = (TypeFree)GetProcAddress(cranLib, "free");
		if (freePtr) {
			(*freePtr)(a_ptr);
		}
		FreeLibrary(cranLib);
	}
}
CPPUTILS_DLL_PRIVATE void* CrInvAllocFree_malloc_default(size_t a_count) {
	void* pReturn = CPPUTILS_NULL;
	HMODULE cranLib = LoadLibraryA(CRASH_INVEST_CRUN_LIB);
	if (cranLib) {
		TypeMalloc mallocPtr = (TypeMalloc)GetProcAddress(cranLib, "malloc");
		if (mallocPtr) {
			pReturn = (*mallocPtr)(a_count);
		}
		FreeLibrary(cranLib);
	}

	return pReturn;
}
CPPUTILS_DLL_PRIVATE void* CrInvAllocFree_realloc_default(void* a_ptr, size_t a_count) {
	void* pReturn = CPPUTILS_NULL;
	HMODULE cranLib = LoadLibraryA(CRASH_INVEST_CRUN_LIB);
	if (cranLib) {
		TypeRealloc reallocPtr = (TypeRealloc)GetProcAddress(cranLib, "realloc");
		if (reallocPtr) {
			pReturn = (*reallocPtr)(a_ptr, a_count);
		}
		FreeLibrary(cranLib);
	}

	return pReturn;
}
CPPUTILS_DLL_PRIVATE void* CrInvAllocFree_calloc_default(size_t a_nmemb, size_t a_size) {
	void* pReturn = CPPUTILS_NULL;
	HMODULE cranLib = LoadLibraryA(CRASH_INVEST_CRUN_LIB);
	if (cranLib) {
		TypeCalloc callocPtr = (TypeCalloc)GetProcAddress(cranLib, "calloc");
		if (callocPtr) {
			pReturn = (*callocPtr)(a_nmemb, a_size);
		}
		FreeLibrary(cranLib);
	}

	return pReturn;
}


#define CRASH_INVEST_INTERFACE_NOT_KNOWN
CRASH_INVEST_INTERFACE_NOT_KNOWN CPPUTILS_DLL_PRIVATE void _windows_crt_all_unknown_functionsWeak(void) {}
#pragma comment(linker, "/alternatename:_RTC_InitBase=_windows_crt_all_unknown_functionsWeak")
#pragma comment(linker, "/alternatename:_RTC_Shutdown=_windows_crt_all_unknown_functionsWeak")
#pragma comment(linker, "/alternatename:_DllMainCRTStartup=_windows_crt_all_unknown_functionsWeak")
#pragma comment(linker, "/alternatename:__security_check_cookie=_windows_crt_all_unknown_functionsWeak")


static HMODULE  s_cranLib = CPPUTILS_NULL;
typedef int (*TypeAtexit)(void(__cdecl* func)(void));
static void CrInvAllocFreeInitializationRoutineStatic(void);
static void CrInvAllocFreeCleanupRoutineStatic(void);

#if 0

#pragma section(".CRT$XCU",read)
#define CR_INV_INITIALIZER_RAW_(f,p) \
        static void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" p #f "_")) \
        static void f(void)

#if defined(_WIN64) || defined(_M_ARM)
#define CR_INV_ALLOC_FREE_INITIALIZER(f) CR_INV_INITIALIZER_RAW_(f,"")
#else
#define CR_INV_ALLOC_FREE_INITIALIZER(f) CR_INV_INITIALIZER_RAW_(f,"_")
#endif


CR_INV_ALLOC_FREE_INITIALIZER(CrInvAllocFreeInitializationRoutine)
{
	CrInvAllocFreeInitializationRoutineStatic();
}

#else

CPPUTILS_DLL_PUBLIC void CrInvAllocFreeInitializationRoutine(void)
{
	CrInvAllocFreeInitializationRoutineStatic();
}

CPPUTILS_DLL_PUBLIC void CrInvAllocFreeCleanupRoutine(void)
{
	CrInvAllocFreeCleanupRoutineStatic();
}

#endif // #if 0


static void CrInvAllocFreeCleanupRoutineStatic(void)
{
	if (s_cranLib) {
		g_callers_malloc = &CrInvAllocFree_malloc_default;
		g_callers_calloc = &CrInvAllocFree_calloc_default;
		g_callers_realloc = &CrInvAllocFree_realloc_default;
		g_callers_free = &CrInvAllocFree_free_default;
		FreeLibrary(s_cranLib);
		s_cranLib = CPPUTILS_NULL;
	}
}



static void CrInvAllocFreeInitializationRoutineStatic(void)
{
	TypeAtexit atExitPtr = CPPUTILS_NULL;

	if (s_cranLib) { return; }

	s_cranLib = LoadLibraryA(CRASH_INVEST_CRUN_LIB);
	if (s_cranLib) {
		TypeMalloc mallocPtr;
		TypeCalloc callocPtr;
		TypeRealloc reallocPtr;
		TypeFree freePtr;
		mallocPtr = (TypeMalloc)GetProcAddress(s_cranLib, "malloc");
		callocPtr = (TypeCalloc)GetProcAddress(s_cranLib, "calloc");
		reallocPtr = (TypeRealloc)GetProcAddress(s_cranLib, "realloc");
		freePtr = (TypeFree)GetProcAddress(s_cranLib, "free");
		atExitPtr = (TypeAtexit)GetProcAddress(s_cranLib, "atexit");
		if (mallocPtr && callocPtr && reallocPtr && freePtr) {
			g_callers_malloc = mallocPtr;
			g_callers_calloc = callocPtr;
			g_callers_realloc = reallocPtr;
			g_callers_free = freePtr;
		}
		else {
			FreeLibrary(s_cranLib);
			s_cranLib = CPPUTILS_NULL;
		}
	}

	if (atExitPtr) {
		(*atExitPtr)(&CrInvAllocFreeCleanupRoutineStatic);
	}
}



CPPUTILS_END_C


#endif  // #ifdef _WIN32
