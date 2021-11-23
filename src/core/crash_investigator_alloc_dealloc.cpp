//
// file:		crash_investigator_memory_items.cpp
// path:		src/core/crash_investigator_memory_items.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL

//#define USE_CPPUTILS

#include "crash_investigator_alloc_dealloc.hpp"
#include <cpputils/enums.hpp>
//#include <unordered_map>
#if defined(USE_CPPUTILS)
#include <cpputils/hashtbl.hpp>
#else
#include <cpputilsm/hashitemsbyptr.hpp>
#endif
#include <mutex>
#include <memory>
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

#ifndef MY_NEW_PRINTF
//#define MY_NEW_PRINTF	printf
#define MY_NEW_PRINTF(...)
#endif


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
	void*			realAddress;
};

static thread_local bool s_bIsAllocingOrDeallocing = false;
//static bool s_bIsAllocingOrDeallocing = false;
class IsAllocingHandler{
public:
	IsAllocingHandler(){s_bIsAllocingOrDeallocing=true;}
	~IsAllocingHandler(){s_bIsAllocingOrDeallocing=false;}
};

#ifdef USE_CPPUTILS
typedef cpputils::hashtbl::Base<void*,SMemoryItem>  TypeHashTbl;
#else
typedef cpputilsm::HashItemsByPtr<void*,SMemoryItem,crash_investigator::malloc,crash_investigator::free>  TypeHashTbl;
#endif
static TypeHashTbl	s_memoryItems;
static std::mutex	s_mutexForMap;


CRASH_INVEST_DLL_PRIVATE void* TestOperatorNew  ( size_t a_count, MemoryType a_memoryType, bool a_bThrow )
{
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::malloc(a_count);}
	IsAllocingHandler aHandler;
	
	if(!a_count){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}
	
	void* pReturn = ::crash_investigator::malloc(a_count);
	if(!pReturn){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}
	
	const SMemoryItem aItem({a_memoryType,MemoryStatus::Allocated,pReturn});
	
	std::lock_guard<std::mutex> aGuard(s_mutexForMap);
	s_memoryItems.AddOrReplaceEntry(pReturn,aItem);
	return pReturn;
}

#ifdef CRASH_INVEST_CPP_17_DEFINED

CRASH_INVEST_DLL_PRIVATE void* TestOperatorNewAligned  ( size_t a_count, MemoryType a_memoryType, bool a_bThrow, size_t a_align )
{
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::malloc(a_count);}
	IsAllocingHandler aHandler;
	
	if(!a_count){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}
	
	if (a_align < __STDCPP_DEFAULT_NEW_ALIGNMENT__)  {
		a_align=__STDCPP_DEFAULT_NEW_ALIGNMENT__;
	}
	else{
		//printf("!!!!!!!!!!! alignment new is called count=%d, (all=%d), __STDCPP_DEFAULT_NEW_ALIGNMENT__=%d!\n",
		//	   (int)a_count, (int)a_align,__STDCPP_DEFAULT_NEW_ALIGNMENT__);
		fflush(stdout);
		uint64_t ullnAlign = static_cast<uint64_t>(a_align)-1;
		ullnAlign |= (ullnAlign >> 1);
		ullnAlign |= (ullnAlign >> 2);
		ullnAlign |= (ullnAlign >> 4);
		ullnAlign |= (ullnAlign >> 8);
		ullnAlign |= (ullnAlign >> 16);
		ullnAlign |= (ullnAlign >> 32);
		++ullnAlign;
		if(static_cast<size_t>(ullnAlign)!=a_align){
			//fprintf(stderr,"6. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Wrong alignment size (%d). "
			//			   "Increasing to %d\n",static_cast<int>(a_align),static_cast<int>(ullnAlign));
			//fflush(stderr);
			//exit(1);
			a_align = static_cast<size_t>(ullnAlign);
		}
	}
	
	size_t actually_allocating = a_count + a_align + sizeof(void*);
	void* pReturn = ::crash_investigator::malloc(actually_allocating);
	if(!pReturn){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}
	
	const SMemoryItem aItem({a_memoryType,MemoryStatus::Allocated,pReturn});
	MY_NEW_PRINTF("line:%d\n",__LINE__);
	
	if (std::align(a_align, a_count, pReturn, actually_allocating)){
		MY_NEW_PRINTF("line:%d\n",__LINE__);
		std::lock_guard<std::mutex> aGuard(s_mutexForMap);
		//s_memoryItems[pReturn] = aItem;
		s_memoryItems.AddOrReplaceEntry(pReturn,aItem);
		MY_NEW_PRINTF("line:%d\n",__LINE__);
		return pReturn;
	}
	
	if(a_bThrow){throw ::std::bad_alloc();}
	
	return CRASH_INVEST_NULL;	
}


#endif  // #ifdef CRASH_INVEST_CPP_17_DEFINED


CRASH_INVEST_DLL_PRIVATE void TestOperatorDelete( void* a_ptr, MemoryType a_typeExpected ) CRASH_INVEST_NOEXCEPT
{
	if(s_bIsAllocingOrDeallocing){ ::crash_investigator::free(a_ptr);return;} // not handling here
	IsAllocingHandler aHandler;
	
	TypeHashTbl::iterator memItemIter;
	void* pToDelete=CRASH_INVEST_NULL;
	
	{
		std::lock_guard<std::mutex> aGuard(s_mutexForMap);
		
		memItemIter = s_memoryItems.FindEntry(a_ptr);
		if(memItemIter == TypeHashTbl::s_endIter){ ::crash_investigator::free(a_ptr);return;} // this is some early memory, leave this
		//if(memItemIter == TypeHashTbl::s_endIter){ return;}
		
		// we will  check 2 things a) if we used new, b) if this buffer is not deleted before
		if(memItemIter->second.status!=MemoryStatus::Allocated){
			fprintf(stderr,"1. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete already deleted memory! Returning without action to prevent crash\n");
			fflush(stderr);
			exit(1);
			return;
		}
		
		if(memItemIter->second.type!=a_typeExpected){	
			//fprintf(stderr,"2. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete using wrong `delete`(expected: %s)! Returning without action to prevent crash\n", a_typeExpected.toString());
			fprintf(stderr,"2. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete using wrong `delete`(expected: %d)! Returning without action to prevent crash\n", static_cast<int>(a_typeExpected));
			fflush(stderr);
			exit(1);
			return;
		}
		
		pToDelete = memItemIter->second.realAddress;
		memItemIter->second.status = MemoryStatus::Deallocated;
	}
	
	::crash_investigator::free(pToDelete);
}


CRASH_INVEST_DLL_PRIVATE void* TestOperatorReAlloc  ( void* a_ptr, size_t a_count )
{
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::realloc(a_ptr,a_count);}
	if(!a_ptr){return TestOperatorNew(a_count,MemoryType::Malloc,false);}
	IsAllocingHandler aHandler;
	
	if(!a_count){return CRASH_INVEST_NULL;}
	
	void* pReturn;
	TypeHashTbl::iterator memItemIter;
	
	{
		std::lock_guard<std::mutex> aGuard(s_mutexForMap);
		
		memItemIter = s_memoryItems.FindEntry(a_ptr);
		if(memItemIter==TypeHashTbl::s_endIter){
			fprintf(stderr,"3. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to realloc wrong memory! Returning without action to prevent crash\n");
			fflush(stderr);
			exit(1);
			return CRASH_INVEST_NULL;
		}
		if(memItemIter->second.type!=MemoryType::Malloc){
			fprintf(stderr,"4. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to realloc wrong type (%d) memory! Returning without action to prevent crash\n",static_cast<int>(memItemIter->second.type));
			fflush(stderr);
			exit(1);
			return CRASH_INVEST_NULL;
		}
		if(memItemIter->second.status!=MemoryStatus::Allocated){
			fprintf(stderr,"5. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to realloc on freed memory! Returning without action to prevent crash\n");
			fflush(stderr);
			exit(1);
			return CRASH_INVEST_NULL;
		}
		
		pReturn = ::crash_investigator::realloc(memItemIter->second.realAddress,a_count) ;
		if(pReturn!=a_ptr){
			SMemoryItem aItem = memItemIter->second;
			s_memoryItems.RemoveEntry(memItemIter);
			aItem.realAddress = pReturn;
			//s_memoryItems[pReturn] = aItem;
			s_memoryItems.AddOrReplaceEntry(pReturn,aItem);
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
		fflush(stderr);
		exit(1);
	}
	s_isInitFunction = false;
	s_isNotInited = false;
	printf("+-+-+-+-+-+-+-+-+-+- Crash investigator lib version 1 +-+-+-+-+-+-+-+-+-+-\n");
	fflush(stdout);
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
