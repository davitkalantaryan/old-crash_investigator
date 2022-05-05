//
// file:		crash_investigator_memory_items.cpp
// path:		src/core/crash_investigator_memory_items.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#ifdef _WIN32
#ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL

#include "crash_investigator_alloc_dealloc.hpp"
#ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#endif

#ifdef _DEBUG
#define CRASH_INVEST_CRUN_LIB	"ucrtbased.dll"
#else
#define CRASH_INVEST_CRUN_LIB	"ucrtbase.dll"
#endif

namespace crash_investigator {

CPPUTILS_DLL_PRIVATE bool SystemSpecificLibInitialRealloc( void*, size_t, void** a_ppReturn ) CPPUTILS_NODISCARD
{
	*a_ppReturn = CPPUTILS_NULL;
	return false;
}


CPPUTILS_DLL_PRIVATE bool SystemSpecificLibInitialDealloc( void* ) CPPUTILS_NOEXCEPT
{
	return false;
}


CPPUTILS_DLL_PRIVATE void* SystemSpecificGlibcRealloc(void* a_ptr, size_t a_count) CPPUTILS_NODISCARD
{
	void* pReturn = CPPUTILS_NULL;
	HMODULE cranLib = LoadLibraryA(CRASH_INVEST_CRUN_LIB);
	if (cranLib) {
		void* (*reallocPtr)(void*, size_t) = (void*(*)(void*,size_t))GetProcAddress(cranLib,"realloc");
		if (reallocPtr) {
			pReturn = (*reallocPtr)(a_ptr,a_count);
		}
		FreeLibrary(cranLib);
	}

	return pReturn;
}


CPPUTILS_DLL_PRIVATE void  SystemSpecificGlibcDealloc(void* a_ptr) CPPUTILS_NOEXCEPT
{
	HMODULE cranLib = LoadLibraryA(CRASH_INVEST_CRUN_LIB);
	if (cranLib) {
		void (*freePtr)(void*) = (void (*)(void*))GetProcAddress(cranLib, "free");
		if (freePtr) {
			(*freePtr)(a_ptr);
		}
		FreeLibrary(cranLib);
	}
}


CPPUTILS_DLL_PRIVATE void* mallocn  ( size_t a_count ) CPPUTILS_NODISCARD
{
	return LocalAlloc(LMEM_FIXED, static_cast<SIZE_T>(a_count));
}


CPPUTILS_DLL_PRIVATE void* reallocn( void* a_ptr, size_t a_count ) CPPUTILS_NODISCARD
{
	return LocalReAlloc(a_ptr, static_cast<SIZE_T>(a_count), LMEM_FIXED);
}


CPPUTILS_DLL_PRIVATE void* callocn(size_t a_nmemb, size_t a_size) CPPUTILS_NODISCARD
{
	const size_t unCount(a_nmemb * a_size);
	return LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, static_cast<SIZE_T>(unCount));
}


CPPUTILS_DLL_PRIVATE void freen( void* a_ptr ) CPPUTILS_NOEXCEPT
{
	LocalFree(a_ptr);
}


} // namespace crash_investigator {

#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
#endif  // #ifndef _WIN32
