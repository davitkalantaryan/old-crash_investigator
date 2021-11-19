//
// file:		test_new_delete.cpp
// path:		src/tests/test_new_delete.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#include <new>
#include <unordered_map>
#include <mutex>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>


enum class MemoryType{
	NotHandled,
	New,
	NewArr,
};

enum class MemoryStatus{
	Allocated,
	Deallocated,
};

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


static inline void* TestOperatorNew  ( std::size_t a_count, MemoryType a_memoryType )
{
	if(!a_count){throw ::std::bad_alloc();}
	
	void* pReturn ( malloc(a_count) );
	if(!pReturn){
		// todo: check if exception throwing is default behaviour
		throw ::std::bad_alloc();
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
	}
		
	std::lock_guard<std::mutex> aGuard(s_mutexForMap);
	s_memoryItems[pReturn] = aItem;
	
	return pReturn;		
}


static inline void TestOperatorDelete( void* a_ptr, MemoryType a_typeExpected ) noexcept
{
	if(s_bIsAllocingOrDeallocing){free(a_ptr);return;} // not handling here
	IsAllocingHandler aHandler;
	std::unordered_map<void*,SMemoryItem>::iterator memItemIter;
	std::lock_guard<std::mutex> aGuard(s_mutexForMap);
	
	memItemIter = s_memoryItems.find(a_ptr);
	if(memItemIter == s_memoryItems.end()){return;} // this is some early memory, leave this
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
			printf("2. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Trying to delete using wrong `delete`(expected: %d)! Returning without action to prevent crash\n", static_cast<int>(a_typeExpected));
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
	return TestOperatorNew(a_count,MemoryType::New);
}


void* operator new []  ( std::size_t a_count )
{
	return TestOperatorNew(a_count,MemoryType::NewArr);
}


void operator delete  ( void* a_ptr ) noexcept
{
	TestOperatorDelete(a_ptr,MemoryType::New);
}


void operator delete [] ( void* a_ptr ) noexcept
{
	TestOperatorDelete(a_ptr,MemoryType::NewArr);
}

void operator delete  ( void* a_ptr, std::size_t ) noexcept
{
	TestOperatorDelete(a_ptr,MemoryType::New);
}


void operator delete [] ( void* a_ptr, std::size_t ) noexcept
{
	TestOperatorDelete(a_ptr,MemoryType::NewArr);
}
