//
// file:		crash_investigator_malloc_free.cpp
// path:		src/core/crash_investigator_malloc_free.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#define CRASH_INVEST_TEST_MSC

#include <crash_investigator/crash_investigator_internal_header.h>

#ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE

#include "crash_investigator_alloc_dealloc.hpp"
#include <crash_investigator/crash_investigator_malloc_free_hook.h>
#include <string.h>


CPPUTILS_BEGIN_C

//#pragma comment(linker, "/alternatename:_pmalloc=_pmallocWin")
//#pragma comment(linker, "/alternatename:_pfree=_pfreeWin")

CRASH_INVEST_EXPORT void* CorrectName(malloc)(size_t a_count)
{
    return ::crash_investigator::TestOperatorAlloc(a_count,::crash_investigator::MemoryType::Malloc,false,1);
}


CRASH_INVEST_EXPORT void* CorrectName(calloc)(size_t a_nmemb, size_t a_size)
{
    return ::crash_investigator::TestOperatorCalloc(a_nmemb, a_size,1);
}


CRASH_INVEST_EXPORT void* CorrectName(realloc)(void* a_ptr, size_t a_count)
{
    return ::crash_investigator::TestOperatorReAlloc(a_ptr,a_count,1);
}


CRASH_INVEST_EXPORT void CorrectName(free)(void* a_ptr)
{
    ::crash_investigator::TestOperatorDelete(a_ptr,::crash_investigator::MemoryType::Malloc,1);
}

#ifdef _MSC_VER

CRASH_INVEST_EXPORT void crash_investiator_new_malloc_init(void) {}


#endif  // #ifdef _MSC_VER


CPPUTILS_END_C

#ifdef _MSC_VER

class MallocFreeIniter {
public:
    MallocFreeIniter() {
        m_callers_malloc = g_callers_malloc;
        m_callers_calloc = g_callers_calloc;
        m_callers_realloc = g_callers_realloc;
        m_callers_free = g_callers_free;

        g_callers_malloc = &(CorrectName(malloc));
        g_callers_calloc = &(CorrectName(calloc));
        g_callers_realloc = &(CorrectName(realloc));
        g_callers_free = &(CorrectName(free));
    }
    ~MallocFreeIniter() {
        g_callers_malloc = m_callers_malloc;
        g_callers_calloc = m_callers_calloc;
        g_callers_realloc = m_callers_realloc;
        g_callers_free = m_callers_free;
    }

private:
    TypeMalloc		m_callers_malloc;
    TypeCalloc		m_callers_calloc;
    TypeRealloc		m_callers_realloc;
    TypeFree		m_callers_free;
}static s_MallocFreeIniter;

#endif  // #ifdef _MSC_VER


#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE
