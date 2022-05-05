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
static inline void* mallocn  ( size_t a_count ) CPPUTILS_NOEXCEPT  {return :: malloc(a_count);}
static inline void* reallocn ( void* a_ptr, size_t a_count ) CPPUTILS_NOEXCEPT { return :: realloc(a_ptr,a_count); }
static inline void* callocn  ( size_t a_nmemb, size_t a_size ) CPPUTILS_NOEXCEPT { return ::calloc(a_nmemb, a_size); }
static inline void  freen    ( void* a_ptr ) CPPUTILS_NOEXCEPT { :: free(a_ptr); }
static inline bool  SystemSpecificLibInitialRealloc  ( void*, size_t, void** a_ppReturn ) CPPUTILS_NODISCARD {*a_ppReturn=CPPUTILS_NULL;return false;}
static inline bool  SystemSpecificLibInitialDealloc( void* ) CPPUTILS_NOEXCEPT {return false;}
static inline void* SystemSpecificGlibcRealloc( void* a_ptr, size_t a_count ) CPPUTILS_NODISCARD { return :: realloc(a_ptr,a_count); }
static inline void  SystemSpecificGlibcDealloc( void* a_ptr ) CPPUTILS_NOEXCEPT { :: free(a_ptr); }
#else
CPPUTILS_DLL_PRIVATE void* mallocn  ( size_t a_count ) CPPUTILS_NODISCARD;
CPPUTILS_DLL_PRIVATE void* reallocn ( void* a_ptr, size_t a_count ) CPPUTILS_NODISCARD;
CPPUTILS_DLL_PRIVATE void* callocn  ( size_t a_nmemb, size_t a_size ) CPPUTILS_NODISCARD;
CPPUTILS_DLL_PRIVATE void  freen    ( void* a_ptr ) CPPUTILS_NOEXCEPT;
CPPUTILS_DLL_PRIVATE bool  SystemSpecificLibInitialRealloc( void* a_ptr, size_t a_count, void** a_ppReturn ) CPPUTILS_NODISCARD ;
CPPUTILS_DLL_PRIVATE bool  SystemSpecificLibInitialDealloc( void* a_ptr ) CPPUTILS_NOEXCEPT ;
CPPUTILS_DLL_PRIVATE void* SystemSpecificGlibcRealloc( void* a_ptr, size_t a_count ) CPPUTILS_NODISCARD ;
CPPUTILS_DLL_PRIVATE void  SystemSpecificGlibcDealloc( void* a_ptr ) CPPUTILS_NOEXCEPT ;
#endif

}  // namespace crash_investigator {
