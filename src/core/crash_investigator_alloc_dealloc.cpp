//
// file:		crash_investigator_memory_items.cpp
// path:		src/core/crash_investigator_memory_items.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL

#include "crash_investigator_alloc_dealloc.hpp"
#include <cpputils/enums.hpp>
#include <unordered_map>
#include <mutex>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#ifdef CRASH_INVEST_DO_NOT_USE_MAL_FREE
#include <malloc.h>
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#endif

namespace crash_investigator {


#ifdef CRASH_INVEST_DO_NOT_USE_MAL_FREE
using :: malloc;
using :: realloc;
using :: free;
#else
static void* malloc  ( size_t a_count ) CRASH_INVEST_NOEXCEPT;
static void* realloc( void* a_ptr, size_t a_count ) CRASH_INVEST_NOEXCEPT;
static void  free( void* a_ptr ) CRASH_INVEST_NOEXCEPT;
#endif


enum class MemoryStatus : uint32_t {
	Allocated,
	Deallocated,
};
//CPPUTILS_ENUM_FAST_RAW(251,MemoryStatus,uint32_t,Allocated,Deallocated);


struct SMemoryItem{
	MemoryType		type;
	MemoryStatus	status;
};

static thread_local bool s_bIsAllocingOrDeallocing = false;
//static bool s_bIsAllocingOrDeallocing = false;
class IsAllocingHandler{
public:
	IsAllocingHandler(){s_bIsAllocingOrDeallocing=true;}
	~IsAllocingHandler(){s_bIsAllocingOrDeallocing=false;}
};

static std::unordered_map<void*,SMemoryItem>	s_memoryItems;
static std::mutex s_mutexForMap;


CRASH_INVEST_DLL_PRIVATE void* TestOperatorNew  ( size_t a_count, MemoryType a_memoryType, bool a_bThrow )
{
	if(!a_count){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::malloc(a_count);}
	
	IsAllocingHandler aHandler;
	
	void* pReturn = ::crash_investigator::malloc(a_count) ;
	if(!pReturn){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}
	
	const SMemoryItem aItem({a_memoryType,MemoryStatus::Allocated});
	
	{
		std::lock_guard<std::mutex> aGuard(s_mutexForMap);
		s_memoryItems[pReturn] = aItem;
	}
	
	return pReturn;	
}


CRASH_INVEST_DLL_PRIVATE void TestOperatorDelete( void* a_ptr, MemoryType a_typeExpected ) CRASH_INVEST_NOEXCEPT
{
	if(s_bIsAllocingOrDeallocing){ ::crash_investigator::free(a_ptr);return;} // not handling here
	
	IsAllocingHandler aHandler;
	std::unordered_map<void*,SMemoryItem>::iterator memItemIter;
	
	{
		std::lock_guard<std::mutex> aGuard(s_mutexForMap);
		
		memItemIter = s_memoryItems.find(a_ptr);
		if(memItemIter == s_memoryItems.end()){ ::crash_investigator::free(a_ptr);return;} // this is some early memory, leave this
		
		// we will  check 2 things a) if we used new, b) if this buffer is not deleted before
		if(memItemIter->second.status!=MemoryStatus::Allocated){
			printf("1. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete already deleted memory! Returning without action to prevent crash\n");
			fflush(stdout);
			return;
		}
		
		if(memItemIter->second.type!=a_typeExpected){	
			//printf("2. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete using wrong `delete`(expected: %s)! Returning without action to prevent crash\n", a_typeExpected.toString());
			printf("2. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete using wrong `delete`(expected: %d)! Returning without action to prevent crash\n", static_cast<int>(a_typeExpected));
			fflush(stdout);
			return;
		}
		
		memItemIter->second.status = MemoryStatus::Deallocated;
	}
	
	::crash_investigator::free(a_ptr);
}


CRASH_INVEST_DLL_PRIVATE void* TestOperatorReAlloc  ( void* a_ptr, size_t a_count )
{
	if(!a_count){return CRASH_INVEST_NULL;}
	if(!a_ptr){return TestOperatorNew(a_count,MemoryType::Malloc,false);}
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::realloc(a_ptr,a_count);}
	
	IsAllocingHandler aHandler;
	void* pReturn;
	std::unordered_map<void*,SMemoryItem>::iterator memItemIter;
	
	{
		std::lock_guard<std::mutex> aGuard(s_mutexForMap);
		
		memItemIter = s_memoryItems.find(a_ptr);
		if(memItemIter==s_memoryItems.end()){
			printf("3. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to realloc wrong memory! Returning without action to prevent crash\n");
			return CRASH_INVEST_NULL;
		}
		if(memItemIter->second.type!=MemoryType::Malloc){
			printf("4. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to realloc wrong type (%d) memory! Returning without action to prevent crash\n",static_cast<int>(memItemIter->second.type));
			fflush(stdout);
			return CRASH_INVEST_NULL;
		}
		if(memItemIter->second.status!=MemoryStatus::Allocated){
			printf("5. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to realloc on freed memory! Returning without action to prevent crash\n");
			fflush(stdout);
			return CRASH_INVEST_NULL;
		}
		
		pReturn = ::crash_investigator::realloc(a_ptr,a_count) ;
		if(pReturn!=a_ptr){
			SMemoryItem aItem = memItemIter->second;
			s_memoryItems.erase(memItemIter);
			s_memoryItems[pReturn] = aItem;
		}
	}
		
	return pReturn;		
}


/*////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

#ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE

#define CRASH_INVEST_MEMORY_HANDLER_SIZE	1048576  // 1MB

typedef void* (*TypeMalloc)(size_t);
typedef void* (*TypeRealloc)(void*,size_t);
typedef void  (*TypeFree)(void*);

static uint8_t			s_vInitFuncBuffer[CRASH_INVEST_MEMORY_HANDLER_SIZE];
static pthread_once_t	s_once_control		= PTHREAD_ONCE_INIT;
static TypeMalloc		s_orig_malloc		= CRASH_INVEST_NULL;
static TypeRealloc		s_orig_realloc		= CRASH_INVEST_NULL;
static TypeFree			s_orig_free			= CRASH_INVEST_NULL;
static bool				s_isInitFunction	= false;
static bool				s_isNotInited		= true;

static void InitFunction(void)
{
	s_isInitFunction = true;
	s_orig_malloc  = reinterpret_cast<TypeMalloc>(dlsym(RTLD_NEXT, "malloc"));
	s_orig_realloc = reinterpret_cast<TypeRealloc>(dlsym(RTLD_NEXT, "realloc"));
	s_orig_free    = reinterpret_cast<TypeFree>(dlsym(RTLD_NEXT, "free"));
	if((!s_orig_malloc)||(!s_orig_realloc)||(!s_orig_free)){
		fprintf(stderr, "Unable to get addresses of original functions (malloc/realloc/free)\n. Application will exit");
		fflush(stdout);
		exit(1);
	}
	s_isInitFunction = false;
	s_isNotInited = false;
}


static void* malloc  ( size_t a_count ) CRASH_INVEST_NOEXCEPT
{
	if(s_isInitFunction){
		if(a_count>CRASH_INVEST_MEMORY_HANDLER_SIZE){return CRASH_INVEST_NULL;}
		return s_vInitFuncBuffer;
	}
	if(s_isNotInited){pthread_once(&s_once_control,&InitFunction);}
	return (*s_orig_malloc)(a_count);
}


static void* realloc( void* a_ptr, size_t a_count ) CRASH_INVEST_NOEXCEPT
{
	if(s_isInitFunction){
		if(a_count>CRASH_INVEST_MEMORY_HANDLER_SIZE){return CRASH_INVEST_NULL;}
		return s_vInitFuncBuffer;
	}
	if(s_isNotInited){pthread_once(&s_once_control,&InitFunction);}
	return (*s_orig_realloc)(a_ptr,a_count);
}


static void free( void* a_ptr ) CRASH_INVEST_NOEXCEPT
{
	if(s_isInitFunction){
		return;
	}
	if(s_isNotInited){pthread_once(&s_once_control,&InitFunction);}
	(*s_orig_free)(a_ptr);
}

#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE



} // namespace crash_investigator {

#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
