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

#if defined(_WIN64) || defined(_M_ARM)
#define CR_INV_FNAME_PREFIX ""
#define CR_INV_DS_FNAME_POSTFIX
#define CR_INV_SEC_CH_FNC_NAME	"__security_check_cookie"
#else
#define CR_INV_FNAME_PREFIX "_"
#define CR_INV_DS_FNAME_POSTFIX	"@12"
#define CR_INV_SEC_CH_FNC_NAME	"@__security_check_cookie@4"
#endif


static void  CrInvAllocFree_free_default(void* a_ptr){
	HMODULE cranLib = LoadLibraryA(CRASH_INVEST_CRUN_LIB);
	if (cranLib) {
		TypeFree freePtr = (TypeFree)GetProcAddress(cranLib, "free");
		if (freePtr) {
			(*freePtr)(a_ptr);
		}
		FreeLibrary(cranLib);
	}
}
static void* CrInvAllocFree_malloc_default(size_t a_count) {
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
static void* CrInvAllocFree_realloc_default(void* a_ptr, size_t a_count) {
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
static void* CrInvAllocFree_calloc_default(size_t a_nmemb, size_t a_size) {
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
CRASH_INVEST_INTERFACE_NOT_KNOWN CPPUTILS_DLL_PRIVATE void windows_crt_all_unknown_functionsWeak(void) {}
#pragma comment(linker, "/alternatename:" CR_INV_FNAME_PREFIX "_RTC_Shutdown=" CR_INV_FNAME_PREFIX "windows_crt_all_unknown_functionsWeak")
#pragma comment(linker, "/alternatename:" CR_INV_FNAME_PREFIX "_RTC_InitBase=" CR_INV_FNAME_PREFIX "windows_crt_all_unknown_functionsWeak")
#pragma comment(linker, "/alternatename:" CR_INV_FNAME_PREFIX "_DllMainCRTStartup" CR_INV_DS_FNAME_POSTFIX "=" CR_INV_FNAME_PREFIX "windows_crt_all_unknown_functionsWeak")
#ifndef _WIN64
#ifdef _M_ARM
#pragma comment(linker, "/alternatename:memset=CrInvAllocFree_memset")
#else
#pragma comment(linker, "/alternatename:__RTC_CheckEsp=_windows_crt_all_unknown_functionsWeak")
#endif
#endif
#ifndef _DEBUG
#pragma comment(linker, "/alternatename:" CR_INV_SEC_CH_FNC_NAME "=" CR_INV_FNAME_PREFIX "windows_crt_all_unknown_functionsWeak")
#endif


static HMODULE  s_cranLib = CPPUTILS_NULL;
typedef int (*TypeAtexit)(void(__cdecl* func)(void));
static void CrInvAllocFreeInitializationRoutineStatic(void);
static void CrInvAllocFreeCleanupRoutineStatic(void);

#if 0

#pragma section(".CRT$XCU",read)
#define CR_INV_ALLOC_FREE_INITIALIZER(f) \
        static void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" CR_INV_FNAME_PREFIX #f "_")) \
        static void f(void)


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


static struct SCrInvAllFreeFunctions s_fncsPointerCrInvAlloc = {
	&CrInvAllocFree_free_default,
	&CrInvAllocFree_malloc_default,
	&CrInvAllocFree_calloc_default,
	&CrInvAllocFree_realloc_default
};

CPPUTILS_DLL_PUBLIC struct SCrInvAllFreeFunctions* s_pCrInvAllocFncsPtrs = &s_fncsPointerCrInvAlloc;
static struct SCrInvAllFreeFunctions* s_pCrInvAllocFncsPtrsAllocated = CPPUTILS_NULL;

static void CrInvAllocFreeCleanupRoutineStatic(void)
{
	if (s_cranLib) {
		s_pCrInvAllocFncsPtrs = &s_fncsPointerCrInvAlloc;
		HeapFree(GetProcessHeap(), 0, s_pCrInvAllocFncsPtrsAllocated);
		s_pCrInvAllocFncsPtrsAllocated = CPPUTILS_NULL;
		FreeLibrary(s_cranLib);
		s_cranLib = CPPUTILS_NULL;
	}
}



static void CrInvAllocFreeInitializationRoutineStatic(void)
{
	struct SCrInvAllFreeFunctions* pFncs;
	static HMODULE cranLib;
	struct SCrInvAllFreeFunctions* pInitVal;
	TypeAtexit atExitPtr = CPPUTILS_NULL;

	if (s_cranLib) { return; }

	cranLib = LoadLibraryA(CRASH_INVEST_CRUN_LIB);
	if (!cranLib) { return; }
	
	pFncs = (struct SCrInvAllFreeFunctions*)HeapAlloc(GetProcessHeap(), 0, sizeof(struct SCrInvAllFreeFunctions));
	if (!pFncs) { return; }

	pFncs->m_free = (TypeFree)GetProcAddress(cranLib, "free");
	pFncs->m_malloc = (TypeMalloc)GetProcAddress(cranLib, "malloc");
	pFncs->m_calloc = (TypeCalloc)GetProcAddress(cranLib, "calloc");
	pFncs->m_realloc = (TypeRealloc)GetProcAddress(cranLib, "realloc");
	atExitPtr = (TypeAtexit)GetProcAddress(cranLib, "atexit");
	if ((!pFncs->m_free) || (!pFncs->m_malloc) || (!pFncs->m_calloc) || (!pFncs->m_realloc)) {
		//g_callers_malloc = mallocPtr;
		//g_callers_calloc = callocPtr;
		//g_callers_realloc = reallocPtr;
		//g_callers_free = freePtr;
		FreeLibrary(cranLib);
		return;
	}

	// todo: implement this
	pInitVal = (struct SCrInvAllFreeFunctions*)InterlockedCompareExchangePointer((PVOID)s_pCrInvAllocFncsPtrs, (PVOID)pFncs, (PVOID)&s_fncsPointerCrInvAlloc);
	if (pInitVal == (&s_fncsPointerCrInvAlloc)) {
		// some other thread make this before as
		HeapFree(GetProcessHeap(), 0, pFncs);
		FreeLibrary(cranLib);
		return;
	}

	s_pCrInvAllocFncsPtrsAllocated = pFncs;

	if (atExitPtr) {
		(*atExitPtr)(&CrInvAllocFreeCleanupRoutineStatic);
	}

}

#if !defined(_WIN64) && defined(_M_ARM)

CPPUTILS_DLL_PRIVATE void* CrInvAllocFree_memset(void* a_dest, int a_c, size_t a_count)
{
	size_t i = 0;
	char* pcData = CPPUTILS_STATIC_CAST(char*, a_dest);
	for (;i<a_count;) {
		pcData[i] = CPPUTILS_STATIC_CAST(char, a_c);
	}
}

#endif  // #if !defined(_WIN64) && defined(_M_ARM)



CPPUTILS_END_C


#endif  // #ifdef _WIN32
