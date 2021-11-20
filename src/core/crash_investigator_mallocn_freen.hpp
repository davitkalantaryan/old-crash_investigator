//
// file:		crash_investigator_memory_items.hpp
// path:		src/core/crash_investigator_memory_items.hpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#pragma once

#include <crash_investigator/crash_investigator_internal_header.h>
#include <stddef.h>
#ifdef CRASH_INVEST_DO_NOT_USE_MAL_FREE
#include <stdlib.h>
#endif


namespace crash_investigator {

#ifdef CRASH_INVEST_DO_NOT_USE_MAL_FREE
using ::malloc;
using ::free;
using ::realloc;
#else
CRASH_INVEST_DLL_PRIVATE void* malloc  ( size_t a_count ) CRASH_INVEST_NOEXCEPT;
CRASH_INVEST_DLL_PRIVATE void* realloc( void* a_ptr, size_t a_count ) CRASH_INVEST_NOEXCEPT;
CRASH_INVEST_DLL_PRIVATE void  free( void* a_ptr ) CRASH_INVEST_NOEXCEPT;
#endif


} // namespace crash_investigator {
