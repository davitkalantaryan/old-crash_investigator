
#include <crash_investigator/crash_investigator_internal_header.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

//#include "../tests/malloc_free/crash_investigator_malloc_free_hook.h"

CRASH_INVEST_BEGIN_C

//void CrashInvestDummy2(void);

CRASH_INVEST_END_C

//static bool CrashInvestHookClbk(const CrashInvestClbkArg* data)
//{
//	printf("CrashInvestHookClbk\n");
//	return true;
//}

#if 1

//#pragma comment (lib,"initial_malloc_free.lib")
//#pragma comment (lib,"second_malloc_free.lib")

CRASH_INVEST_DLL_PRIVATE void* CrashInvestAllocMemory(size_t a_count)
{
	return LocalAlloc(LMEM_FIXED, CRASH_INVEST_STATIC_CAST(SIZE_T, a_count));
}


CRASH_INVEST_DLL_PRIVATE void* CrashInvestReallocMemory(void* a_ptr, size_t a_count)
{
	return LocalReAlloc(a_ptr, CRASH_INVEST_STATIC_CAST(SIZE_T, a_count), LMEM_FIXED);
}


CRASH_INVEST_DLL_PRIVATE void* CrashInvestCallocMemory(size_t a_nmemb, size_t a_size)
{
	const size_t unCount = a_nmemb * a_size;
	return LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, CRASH_INVEST_STATIC_CAST(SIZE_T, unCount));
}


CRASH_INVEST_DLL_PRIVATE void CrashInvestFreeMemory(void* a_ptr)
{
	LocalFree(a_ptr);
}
CRASH_INVEST_DLL_PRIVATE void Dummy(void) {}



int main()
{
	//CrashInvestSetNewHookClbk(CrashInvestHookClbk);
	//CrashInvestMallocHookDummy();
	//CrashInvestDummy2();

	const void* pFncIn;
	void* pFncD;
	size_t injectSize;

	pFncIn = (const void*)(&CrashInvestReallocMemory);
	pFncD = (void*)(&malloc);
	//injectSize = (char*)((void*)(&CrashInvestAllocMemory)) - (char*)((void*)(&CrashInvestReallocMemory));
	injectSize = 100;
	memcpy(pFncD, pFncIn, injectSize);


	//HMODULE hMod = LoadLibraryA("initial_malloc_free.dll");
	//
	//void* pMemory = malloc(10);
	//free(pMemory);
	//
	//if (hMod) {
	//	FreeLibrary(hMod);
	//}

	return 0;
}

#endif



CRASH_INVEST_BEGIN_C



void* mallocCrashInvestWin(size_t a_unSize)
{
	return LocalAlloc(LMEM_FIXED,static_cast<SIZE_T>(a_unSize));
}


void freeCrashInvestWin(void* a_ptr)
{
	LocalFree(a_ptr);
}

CRASH_INVEST_END_C
