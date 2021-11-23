//
// file:		crash_investigator_malloc_free.cpp
// path:		src/core/crash_investigator_malloc_free.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#include <crash_investigator/crash_investigator_internal_header.h>

#ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE

#include "crash_investigator_alloc_dealloc.hpp"
#include <string.h>


CRASH_INVEST_BEGIN_C


CRASH_INVEST_DLL_PUBLIC void* malloc(size_t a_count)
{
	return ::crash_investigator::TestOperatorNewAligned(a_count,::crash_investigator::MemoryType::Malloc,false);
}


CRASH_INVEST_DLL_PUBLIC void* calloc(size_t a_nmemb, size_t a_size)
{
	const size_t unCount ( a_nmemb * a_size );
	void* pReturn = ::crash_investigator::TestOperatorNewAligned(unCount,::crash_investigator::MemoryType::Malloc,false,a_size);
	if(pReturn){
		memset(pReturn,0,unCount);
	}
	return pReturn;
}


CRASH_INVEST_DLL_PUBLIC void* realloc(void* a_ptr, size_t a_count)
{
	return ::crash_investigator::TestOperatorReAlloc(a_ptr,a_count);
}


CRASH_INVEST_DLL_PUBLIC void free(void* a_ptr)
{
	::crash_investigator::TestOperatorDelete(a_ptr,::crash_investigator::MemoryType::Malloc);
}


CRASH_INVEST_END_C


#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE
