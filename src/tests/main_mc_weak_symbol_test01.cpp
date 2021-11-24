
#include <crash_investigator/crash_investigator_internal_header.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
//#include <malloc.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include "../tests/malloc_free/crash_investigator_malloc_free_hook.h"

CRASH_INVEST_BEGIN_C

void CrashInvestDummy2(void);

CRASH_INVEST_END_C

static bool CrashInvestHookClbk(const CrashInvestClbkArg* data)
{
	printf("CrashInvestHookClbk\n");
	return true;
}

#if 1

//#pragma comment (lib,"initial_malloc_free.lib")
//#pragma comment (lib,"second_malloc_free.lib")

int main()
{
	//CrashInvestSetNewHookClbk(CrashInvestHookClbk);
	//CrashInvestMallocHookDummy();
	//CrashInvestDummy2();

	HMODULE hMod = LoadLibraryA("initial_malloc_free.dll");

	void* pMemory = malloc(10);
	free(pMemory);

	if (hMod) {
		FreeLibrary(hMod);
	}

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
