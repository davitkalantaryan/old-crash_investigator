//
// file:		crash_investigator_alloc_dealloc.cpp
// path:		src/core/crash_investigator.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#include <crash_investigator/crash_investigator_internal_header.h>
#include <cpputils/enums.hpp>
#include <new>
#include <unordered_map>
#include <mutex>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>


//enum class MemoryType{
//	NotHandled,
//	New,
//	NewArr,
//};
//enum class MemoryStatus{
//	Allocated,
//	Deallocated,
//};
CPPUTILS_ENUM_FAST_RAW(142,MemoryType,uint32_t,NotHandled,New,NewArr);
CPPUTILS_ENUM_FAST_RAW(251,MemoryStatus,uint32_t,Allocated,Deallocated);


struct SMemoryItem{
	MemoryType		type;
	MemoryStatus	status;
};

static thread_local bool s_bIsAllocingOrDeallocing = false;
class IsAllocingHandler{
public:
	IsAllocingHandler(){m_bIsLocker=false;}
	~IsAllocingHandler(){if(m_bIsLocker){s_bIsAllocingOrDeallocing=false;}}
	void Lock(){s_bIsAllocingOrDeallocing=true;m_bIsLocker=true;}
private:
	bool m_bIsLocker;
};

static std::unordered_map<void*,SMemoryItem>	s_memoryItems;
static std::mutex s_mutexForMap;


static inline void* TestOperatorNew  ( std::size_t a_count, MemoryType a_memoryType, bool a_bThrow )
{
	if(!a_count){throw ::std::bad_alloc();}
	
	void* pReturn = malloc(a_count) ;
	if(!pReturn){
		// todo: check if exception throwing is default behaviour
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}
	
	SMemoryItem aItem;
	aItem.status = MemoryStatus::Allocated;
	IsAllocingHandler aHandler;
	
	if(s_bIsAllocingOrDeallocing){ // not handling here
		aItem.type = MemoryType::NotHandled;
	}
	else{
		aHandler.Lock();
		aItem.type = a_memoryType;
		std::lock_guard<std::mutex> aGuard(s_mutexForMap);
		s_memoryItems[pReturn] = aItem;
	}
		
	//std::lock_guard<std::mutex> aGuard(s_mutexForMap);
	//s_memoryItems[pReturn] = aItem;
	
	return pReturn;		
}


static inline void TestOperatorDelete( void* a_ptr, MemoryType a_typeExpected ) noexcept
{
	if(s_bIsAllocingOrDeallocing){free(a_ptr);return;} // not handling here
	IsAllocingHandler aHandler;
	std::unordered_map<void*,SMemoryItem>::iterator memItemIter;
	std::lock_guard<std::mutex> aGuard(s_mutexForMap);
	
	memItemIter = s_memoryItems.find(a_ptr);
	//if(memItemIter == s_memoryItems.end()){return;} // this is some early memory, leave this
	// todo: comment below and uncomment upper
	if(memItemIter == s_memoryItems.end()){free(a_ptr);return;} // this is some early memory, leave this
	SMemoryItem aItem = memItemIter->second;
	
	// we will  check 2 things a) if we used new, b) if this buffer is not deleted before
	if(aItem.status!=MemoryStatus::Allocated){
		printf("1. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete already deleted memory! Returning without action to prevent crash\n");
		return;
	}
	
	switch (aItem.type) {
	case MemoryType::NotHandled:
		s_memoryItems.erase(memItemIter);
		free(a_ptr);
		return;
	default:
		if(aItem.type!=a_typeExpected){	
			printf("2. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete using wrong `delete`(expected: %s)! Returning without action to prevent crash\n", a_typeExpected.toString());
			return;
		}
		break;
	}
	
	aItem.status = MemoryStatus::Deallocated;
	memItemIter->second = aItem;
	free(a_ptr);
}


void* operator new  ( std::size_t a_count )
{
	return TestOperatorNew(a_count,MemoryType::New, true);
}


void* operator new []  ( std::size_t a_count )
{
	return TestOperatorNew(a_count,MemoryType::NewArr, true);
}

void* operator new  ( std::size_t a_count, const std::nothrow_t& ) CRASH_INVEST_NODISCARD
{
	return TestOperatorNew(a_count,MemoryType::New, false);
}


void* operator new []  ( std::size_t a_count ) CRASH_INVEST_NODISCARD
{
	return TestOperatorNew(a_count,MemoryType::NewArr, false);
}


void operator delete  ( void* a_ptr ) noexcept
{
	TestOperatorDelete(a_ptr,MemoryType::New);
}


void operator delete [] ( void* a_ptr ) noexcept
{
	TestOperatorDelete(a_ptr,MemoryType::NewArr);
}

void operator delete  ( void* a_ptr, std::size_t ) CRASH_INVEST_NOEXCEPT
{
	TestOperatorDelete(a_ptr,MemoryType::New);
}


void operator delete [] ( void* a_ptr, std::size_t ) CRASH_INVEST_NOEXCEPT
{
	TestOperatorDelete(a_ptr,MemoryType::NewArr);
}

#ifdef CRASH_INVEST_CPP_17_DEFINED

void* operator new  ( std::size_t a_count, std::align_val_t )
{
	return TestOperatorNew(a_count,MemoryType::New, true);
}


void* operator new []  ( std::size_t a_count, std::align_val_t )
{
	return TestOperatorNew(a_count,MemoryType::NewArr, true);
}

void* operator new  ( std::size_t a_count, std::align_val_t, const std::nothrow_t& ) CRASH_INVEST_NODISCARD
{
	return TestOperatorNew(a_count,MemoryType::New, true);
}


void* operator new []  ( std::size_t a_count, std::align_val_t, const std::nothrow_t& ) CRASH_INVEST_NODISCARD
{
	return TestOperatorNew(a_count,MemoryType::NewArr, true);
}

#endif  // #ifdef CRASH_INVEST_CPP_17_DEFINED
