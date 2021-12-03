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

namespace crash_investigator {

CRASH_INVEST_DLL_PRIVATE bool SystemSpecificEarlyRealloc  ( void*, size_t, void** a_ppReturn ) CRASH_INVEST_NODISCARD
{
	*a_ppReturn = CRASH_INVEST_NULL;
	return false;
}


CRASH_INVEST_DLL_PRIVATE bool SystemSpecificEarlyDealloc( void* ) CRASH_INVEST_NOEXCEPT
{
	return false;
}


CRASH_INVEST_DLL_PRIVATE void* mallocn  ( size_t a_count ) CRASH_INVEST_NODISCARD
{
	return LocalAlloc(LMEM_FIXED, static_cast<SIZE_T>(a_count));
}


CRASH_INVEST_DLL_PRIVATE void* reallocn( void* a_ptr, size_t a_count ) CRASH_INVEST_NODISCARD
{
	return LocalReAlloc(a_ptr, static_cast<SIZE_T>(a_count), LMEM_FIXED);
}


CRASH_INVEST_DLL_PRIVATE void* callocn(size_t a_nmemb, size_t a_size) CRASH_INVEST_NODISCARD
{
	const size_t unCount(a_nmemb * a_size);
	return LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, static_cast<SIZE_T>(unCount));
}


CRASH_INVEST_DLL_PRIVATE void freen( void* a_ptr ) CRASH_INVEST_NOEXCEPT
{
	LocalFree(a_ptr);
}


} // namespace crash_investigator {

#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
#endif  // #ifndef _WIN32
