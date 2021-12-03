//
// file:		crash_investigator_memory_items.cpp
// path:		src/core/crash_investigator_memory_items.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#ifndef _WIN32
#ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL

#include <crash_investigator/crash_investigator_internal_header.h>
#ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE

#include <cpputilsm/memorypool.hpp>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

namespace crash_investigator {


#define CRASH_INVEST_MEMORY_HANDLER_SIZE2	1048576  // 1MB

typedef void* (*TypeMalloc)(size_t);
typedef void* (*TypeCalloc)(size_t a_nmemb, size_t a_size);
typedef void* (*TypeRealloc)(void*,size_t);
typedef void  (*TypeFree)(void*);

//static uint8_t			s_vInitFuncBuffer[CRASH_INVEST_MEMORY_HANDLER_SIZE];
static cpputilsm::MemoryPool<CRASH_INVEST_MEMORY_HANDLER_SIZE2> s_memPool;

static pthread_once_t	s_once_control		= PTHREAD_ONCE_INIT;
static TypeMalloc		s_orig_malloc		= CRASH_INVEST_NULL;
static TypeCalloc		s_orig_calloc		= CRASH_INVEST_NULL;
static TypeRealloc		s_orig_realloc		= CRASH_INVEST_NULL;
static TypeFree			s_orig_free			= CRASH_INVEST_NULL;
static bool				s_isInitFunction	= false;
static bool				s_isNotInited		= true;

static void InitFunction(void)
{
	s_isInitFunction = true;
	s_orig_malloc  = reinterpret_cast<TypeMalloc>(dlsym(RTLD_NEXT, "malloc"));
	s_orig_calloc  = reinterpret_cast<TypeCalloc>(dlsym(RTLD_NEXT, "calloc"));
	s_orig_realloc = reinterpret_cast<TypeRealloc>(dlsym(RTLD_NEXT, "realloc"));
	s_orig_free    = reinterpret_cast<TypeFree>(dlsym(RTLD_NEXT, "free"));
	if((!s_orig_malloc)||(!s_orig_realloc)||(!s_orig_free)){
		fprintf(stderr, "Unable to get addresses of original functions (malloc/realloc/free)\n. Application will exit");
		fflush(stderr);
		exit(1);
	}
	s_isInitFunction = false;
	s_isNotInited = false;
    //printf("+-+-+-+-+-+-+-+-+-+- Crash investigator lib version 3 +-+-+-+-+-+-+-+-+-+-\n");
    //fflush(stdout);
}


CRASH_INVEST_DLL_PRIVATE bool SystemSpecificEarlyRealloc  ( void* a_ptr, size_t a_count, void** a_ppReturn ) CRASH_INVEST_NODISCARD
{
	return s_memPool.Realloc(a_ptr,a_count,a_ppReturn);
}


CRASH_INVEST_DLL_PRIVATE bool SystemSpecificEarlyDealloc( void* a_ptr ) CRASH_INVEST_NOEXCEPT
{
	return s_memPool.Dealloc(a_ptr);
}


CRASH_INVEST_DLL_PRIVATE void* mallocn  ( size_t a_count ) CRASH_INVEST_NODISCARD
{
	if(s_isInitFunction){
		return s_memPool.Alloc(a_count);
	}
	if(s_isNotInited){pthread_once(&s_once_control,&InitFunction);}
	return (*s_orig_malloc)(a_count);
}


CRASH_INVEST_DLL_PRIVATE void* reallocn( void* a_ptr, size_t a_count ) CRASH_INVEST_NODISCARD
{
	if(s_isInitFunction){
		void* pReturn;
		if(s_memPool.Realloc(a_ptr,a_count,&pReturn)){
			return pReturn;
		}
		return CRASH_INVEST_NULL;
	}
	if(s_isNotInited){pthread_once(&s_once_control,&InitFunction);}
	return (*s_orig_realloc)(a_ptr,a_count);
}


CRASH_INVEST_DLL_PRIVATE void* callocn  ( size_t a_nmemb, size_t a_size ) CRASH_INVEST_NODISCARD
{
	if(s_isInitFunction){
		return s_memPool.Calloc(a_nmemb,a_size);
	}
	if(s_isNotInited){pthread_once(&s_once_control,&InitFunction);}
	return (*s_orig_calloc)(a_nmemb,a_size);
}


CRASH_INVEST_DLL_PRIVATE void freen( void* a_ptr ) CRASH_INVEST_NOEXCEPT
{
	if(s_isInitFunction){
		s_memPool.Dealloc(a_ptr);
		return;
	}
	if(s_isNotInited){pthread_once(&s_once_control,&InitFunction);}
	(*s_orig_free)(a_ptr);
}



} // namespace crash_investigator {

#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE
#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
#endif  // #ifndef _WIN32
