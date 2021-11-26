//
// file:		crash_investigator_malloc_free.cpp
// path:		src/core/crash_investigator_malloc_free.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#define CRASH_INVEST_TEST_MSC

#include <crash_investigator/crash_investigator_internal_header.h>

//#if !defined(_MSC_VER) || defined(CRASH_INVEST_USING_STATIC_LIB_OR_OBJECTS) 
#if !defined(_MSC_VER) || defined(CRASH_INVEST_TEST_MSC)
#ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE

#include "crash_investigator_alloc_dealloc.hpp"
#include <string.h>


CRASH_INVEST_BEGIN_C

//#pragma comment(linker, "/alternatename:_pmalloc=_pmallocWin")
//#pragma comment(linker, "/alternatename:_pfree=_pfreeWin")

CRASH_INVEST_EXPORT void* malloc(size_t a_count)
{
    return ::crash_investigator::TestOperatorAlloc(a_count,::crash_investigator::MemoryType::Malloc,false,1);
}


CRASH_INVEST_EXPORT void* calloc(size_t a_nmemb, size_t a_size)
{
    return ::crash_investigator::TestOperatorCalloc(a_nmemb, a_size,1);
}


CRASH_INVEST_EXPORT void* realloc(void* a_ptr, size_t a_count)
{
    return ::crash_investigator::TestOperatorReAlloc(a_ptr,a_count,1);
}


CRASH_INVEST_EXPORT void free(void* a_ptr)
{
    ::crash_investigator::TestOperatorDelete(a_ptr,::crash_investigator::MemoryType::Malloc,1);
}


CRASH_INVEST_END_C

#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE
#endif  // #if !defined(_MSC_VER) || defined(CRASH_INVEST_USING_STATIC_LIB_OR_OBJECTS)
