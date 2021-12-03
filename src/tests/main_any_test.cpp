//
// file:		main_any_test.cpp
// path:		src/tests/main_any_test.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 29
//

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <cpputilsm/memorypool.hpp>


#define CRASH_INVEST_MEMORY_HANDLER_SIZE	1048576  // 1MB


int main(int, char* [])
{
	//void* pMemory = malloc(10);
	//realloc(pMemory,100);
	//realloc(pMemory,200);  // this is a potential double free
	//return 0;
	
	
	void *pMem01, *pMem02, *pMem03;
	cpputilsm::MemoryPool<CRASH_INVEST_MEMORY_HANDLER_SIZE> memPool;
	
	//
	pMem01 = memPool.Alloc(100);
	pMem02 = memPool.Alloc(100);
	pMem03 = memPool.Alloc(100);
	
	memPool.Dealloc(pMem03);
	memPool.Dealloc(pMem02);
	memPool.Dealloc(pMem01);
	//
	
	//
	pMem01 = memPool.Alloc(100);
	pMem02 = memPool.Alloc(100);
	pMem03 = memPool.Alloc(100);
	
	memPool.Dealloc(pMem01);
	memPool.Dealloc(pMem02);
	memPool.Dealloc(pMem03);
	//
	
	//
	pMem01 = memPool.Alloc(100);
	memPool.Realloc(pMem01,0,&pMem02);
	memPool.Realloc(pMem02,100,&pMem03);
	memPool.Realloc(pMem03,1000,&pMem01);
	memPool.Realloc(&pMem03,1000,&pMem02);
	
	memPool.Dealloc(pMem01);
	memPool.Dealloc(pMem02);
	//
	
}
