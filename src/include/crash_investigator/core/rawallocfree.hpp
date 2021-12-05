//
// file:		rawallocfree.hpp
// path:		src/include/crash_investigator/core/rawallocfree.hpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 25
//


#pragma once

#include <crash_investigator/crash_investigator_internal_header.h>
#include <stddef.h>
#ifdef CRASH_INVEST_DO_NOT_USE_MAL_FREE
#include <stdlib.h>
#include <malloc.h>
#endif


namespace crash_investigator {

#ifdef CRASH_INVEST_DO_NOT_USE_MAL_FREE
static inline void* mallocn  ( size_t a_count ) CRASH_INVEST_NOEXCEPT  {return :: malloc(a_count);}
static inline void* reallocn ( void* a_ptr, size_t a_count ) CRASH_INVEST_NOEXCEPT { return :: realloc(a_ptr,a_count); }
static inline void* callocn  ( size_t a_nmemb, size_t a_size ) CRASH_INVEST_NOEXCEPT { return ::calloc(a_nmemb, a_size); }
static inline void  freen    ( void* a_ptr ) CRASH_INVEST_NOEXCEPT { return :: free(a_ptr); }
static inline bool  SystemSpecificEarlyRealloc  ( void*, size_t, void** a_ppReturn ) CRASH_INVEST_NODISCARD {*a_ppReturn=CRASH_INVEST_NULL;return false;}
static inline bool  SystemSpecificEarlyDealloc( void* ) CRASH_INVEST_NOEXCEPT {return false;}
#else
CRASH_INVEST_DLL_PRIVATE void* mallocn  ( size_t a_count ) CRASH_INVEST_NODISCARD;
CRASH_INVEST_DLL_PRIVATE void* reallocn ( void* a_ptr, size_t a_count ) CRASH_INVEST_NODISCARD;
CRASH_INVEST_DLL_PRIVATE void* callocn  ( size_t a_nmemb, size_t a_size ) CRASH_INVEST_NODISCARD;
CRASH_INVEST_DLL_PRIVATE void  freen    ( void* a_ptr ) CRASH_INVEST_NOEXCEPT;
CRASH_INVEST_DLL_PRIVATE bool  SystemSpecificLibInitialRealloc( void* a_ptr, size_t a_count, void** a_ppReturn ) CRASH_INVEST_NODISCARD ;
CRASH_INVEST_DLL_PRIVATE bool  SystemSpecificLibInitialDealloc( void* a_ptr ) CRASH_INVEST_NOEXCEPT ;
CRASH_INVEST_DLL_PRIVATE void* SystemSpecificGlibcRealloc( void* a_ptr, size_t a_count ) CRASH_INVEST_NODISCARD ;
CRASH_INVEST_DLL_PRIVATE void  SystemSpecificGlibcDealloc( void* a_ptr ) CRASH_INVEST_NOEXCEPT ;
#endif

}  // namespace crash_investigator {
