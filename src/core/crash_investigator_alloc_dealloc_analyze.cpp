//
// file:		crash_investigator_memory_items.cpp
// path:		src/core/crash_investigator_memory_items.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL


#include "crash_investigator_windows_missing_functions.h"
#include "crash_investigator_alloc_dealloc.hpp"
//#include <cpputils/enums.hpp>
//#include <unordered_map>
#if defined(USE_CPPUTILS)
#include <cpputils/hashtbl.hpp>
#else
#include <cpputilsm/hashitemsbyptr.hpp>
#endif
#include <mutex>
#include <stdio.h>
#ifdef CRASH_INVEST_DO_NOT_USE_MAL_FREE
#include <stdlib.h>
#include <malloc.h>
#endif

namespace crash_investigator {

#ifndef MY_NEW_PRINTF
//#define MY_NEW_PRINTF	printf
#define MY_NEW_PRINTF(...)
#endif


#ifdef CRASH_INVEST_DO_NOT_USE_MAL_FREE
static inline void* mallocn  ( size_t a_count ) CRASH_INVEST_NOEXCEPT  {return :: malloc(a_count);}
static inline void* reallocn ( void* a_ptr, size_t a_count ) CRASH_INVEST_NOEXCEPT { return :: realloc(a_ptr,a_count); }
static inline void* callocn  ( size_t a_nmemb, size_t a_size ) CRASH_INVEST_NOEXCEPT { return ::calloc(a_nmemb, a_size); }
static inline void  freen    ( void* a_ptr ) CRASH_INVEST_NOEXCEPT { return :: free(a_ptr); }
#else
CRASH_INVEST_DLL_PRIVATE void* mallocn  ( size_t a_count ) CRASH_INVEST_NOEXCEPT;
CRASH_INVEST_DLL_PRIVATE void* reallocn ( void* a_ptr, size_t a_count ) CRASH_INVEST_NOEXCEPT;
CRASH_INVEST_DLL_PRIVATE void* callocn  ( size_t a_nmemb, size_t a_size ) CRASH_INVEST_NOEXCEPT;
CRASH_INVEST_DLL_PRIVATE void  freen    ( void* a_ptr ) CRASH_INVEST_NOEXCEPT;
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
typedef cpputilsm::HashItemsByPtr<void*,SMemoryItem,&mallocn,&freen>  TypeHashTbl;
#endif
static TypeHashTbl	s_memoryItems;
static std::mutex	s_mutexForMap;


CRASH_INVEST_DLL_PRIVATE void* TestOperatorAlloc  ( size_t a_count, MemoryType a_memoryType, bool a_bThrow )
{
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::mallocn(a_count);}
	IsAllocingHandler aHandler;
	
	if(!a_count){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}
	
	void* pReturn = ::crash_investigator::mallocn(a_count);
	if(!pReturn){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}
	
	const SMemoryItem aItem({a_memoryType,MemoryStatus::Allocated,pReturn});
	
	std::lock_guard<std::mutex> aGuard(s_mutexForMap);
	s_memoryItems.AddOrReplaceEntry(pReturn,aItem);
	return pReturn;
}


CRASH_INVEST_DLL_PRIVATE void* TestOperatorCalloc(size_t a_nmemb, size_t a_size)
{
	if (s_bIsAllocingOrDeallocing) { return ::crash_investigator::callocn(a_nmemb,a_size); }
	IsAllocingHandler aHandler;

	if ((!a_nmemb)|| (!a_size)) { return CRASH_INVEST_NULL; }

	void* pReturn = ::crash_investigator::callocn(a_nmemb, a_size);
	if (!pReturn) {return CRASH_INVEST_NULL;}

	const SMemoryItem aItem({ MemoryType::Malloc,MemoryStatus::Allocated,pReturn });

	std::lock_guard<std::mutex> aGuard(s_mutexForMap);
	s_memoryItems.AddOrReplaceEntry(pReturn, aItem);
	return pReturn;
}


CRASH_INVEST_DLL_PRIVATE void* TestOperatorReAlloc  ( void* a_ptr, size_t a_count )
{
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::reallocn(a_ptr,a_count);}
	if(!a_ptr){return TestOperatorAlloc(a_count,MemoryType::Malloc,false);}
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
		
		pReturn = ::crash_investigator::reallocn(memItemIter->second.realAddress,a_count) ;
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


CRASH_INVEST_DLL_PRIVATE void TestOperatorDelete(void* a_ptr, MemoryType a_typeExpected) CRASH_INVEST_NOEXCEPT
{
	if (s_bIsAllocingOrDeallocing) { ::crash_investigator::freen(a_ptr); return; } // not handling here
	IsAllocingHandler aHandler;

	TypeHashTbl::iterator memItemIter;
	void* pToDelete = CRASH_INVEST_NULL;

	{
		std::lock_guard<std::mutex> aGuard(s_mutexForMap);

		memItemIter = s_memoryItems.FindEntry(a_ptr);
		if (memItemIter == TypeHashTbl::s_endIter) { ::crash_investigator::freen(a_ptr); return; } // this is some early memory, leave this
		//if(memItemIter == TypeHashTbl::s_endIter){ return;}

		// we will  check 2 things a) if we used new, b) if this buffer is not deleted before
		if (memItemIter->second.status != MemoryStatus::Allocated) {
			fprintf(stderr, "1. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete already deleted memory! Returning without action to prevent crash\n");
			fflush(stderr);
			exit(1);
			return;
		}

		if (memItemIter->second.type != a_typeExpected) {
			//fprintf(stderr,"2. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete using wrong `delete`(expected: %s)! Returning without action to prevent crash\n", a_typeExpected.toString());
			fprintf(stderr, "2. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete using wrong `delete`(expected: %d)! Returning without action to prevent crash\n", static_cast<int>(a_typeExpected));
			fflush(stderr);
			exit(1);
			return;
		}

		pToDelete = memItemIter->second.realAddress;
		memItemIter->second.status = MemoryStatus::Deallocated;
	}

	::crash_investigator::freen(pToDelete);
}


//#define CRASH_INVEST_CPP_17_DEFINED
#ifdef CRASH_INVEST_CPP_17_DEFINED

CRASH_INVEST_DLL_PRIVATE void* TestOperatorNewAligned(size_t a_count, MemoryType a_memoryType, bool a_bThrow, size_t a_align)
{
	if (s_bIsAllocingOrDeallocing) { return ::crash_investigator::mallocn(a_count); }
	IsAllocingHandler aHandler;

	if (!a_count) {
		if (a_bThrow) { throw ::std::bad_alloc(); }
		else { return CRASH_INVEST_NULL; }
	}

	if (a_align < __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
		a_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
	}
	else {
		//printf("!!!!!!!!!!! alignment new is called count=%d, (all=%d), __STDCPP_DEFAULT_NEW_ALIGNMENT__=%d!\n",
		//	   (int)a_count, (int)a_align,__STDCPP_DEFAULT_NEW_ALIGNMENT__);
		fflush(stdout);
		uint64_t ullnAlign = static_cast<uint64_t>(a_align) - 1;
		ullnAlign |= (ullnAlign >> 1);
		ullnAlign |= (ullnAlign >> 2);
		ullnAlign |= (ullnAlign >> 4);
		ullnAlign |= (ullnAlign >> 8);
		ullnAlign |= (ullnAlign >> 16);
		ullnAlign |= (ullnAlign >> 32);
		++ullnAlign;
		if (static_cast<size_t>(ullnAlign) != a_align) {
			//fprintf(stderr,"6. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Wrong alignment size (%d). "
			//			   "Increasing to %d\n",static_cast<int>(a_align),static_cast<int>(ullnAlign));
			//fflush(stderr);
			//exit(1);
			a_align = static_cast<size_t>(ullnAlign);
		}
	}

	size_t actually_allocating = a_count + a_align + sizeof(void*);
	void* pReturn = ::crash_investigator::mallocn(actually_allocating);
	if (!pReturn) {
		if (a_bThrow) { throw ::std::bad_alloc(); }
		else { return CRASH_INVEST_NULL; }
	}

	const SMemoryItem aItem({ a_memoryType,MemoryStatus::Allocated,pReturn });
	MY_NEW_PRINTF("line:%d\n", __LINE__);

	if (std::align(a_align, a_count, pReturn, actually_allocating)) {
		MY_NEW_PRINTF("line:%d\n", __LINE__);
		std::lock_guard<std::mutex> aGuard(s_mutexForMap);
		//s_memoryItems[pReturn] = aItem;
		s_memoryItems.AddOrReplaceEntry(pReturn, aItem);
		MY_NEW_PRINTF("line:%d\n", __LINE__);
		return pReturn;
	}

	if (a_bThrow) { throw ::std::bad_alloc(); }

	return CRASH_INVEST_NULL;
}

#endif  // #ifdef CRASH_INVEST_CPP_17_DEFINED


} // namespace crash_investigator {

#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
