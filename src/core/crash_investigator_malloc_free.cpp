//
// file:		crash_investigator_malloc_free.cpp
// path:		src/core/crash_investigator_malloc_free.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE

#include "crash_investigator_mallocn_freen.hpp"
#include "crash_investigator_alloc_dealloc.hpp"
#include <stdlib.h>
#include <malloc.h>
#include <string.h>



void* malloc  ( size_t a_count )
{
	return crash_investigator::TestOperatorNew(a_count,crash_investigator::MemoryType::Malloc, false);
}


void* calloc(size_t a_nmemb, size_t a_size)
{
	const size_t cunOverallSize(a_nmemb*a_size);
	void* pReturn = crash_investigator::TestOperatorNew(cunOverallSize,crash_investigator::MemoryType::Malloc, false);
	if(!pReturn){return pReturn;}
	memset(pReturn,0,cunOverallSize);
	return pReturn;
}


void* realloc  ( void* a_ptr, size_t a_count )
{
	return crash_investigator::TestOperatorReAlloc(a_ptr,a_count);
}


void free( void* a_ptr )
{
	::crash_investigator::free(a_ptr);
}


#endif // #ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE
