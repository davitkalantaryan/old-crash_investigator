//
// file:		crash_investigator_malloc_free_hook_windows2.c
// path:		src/core/crash_investigator_malloc_free_hook_windows2.c
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 24
//

#if defined(_WIN32)

#include <crash_investigator/crash_investigator_internal_header.h>
#include "crash_investigator_malloc_free_hook.h"


#define CRASH_INVEST_INTERFACE_NOT_KNOWN

CRASH_INVEST_BEGIN_C

typedef void* CrashInvestMutex;

CRASH_INVEST_DLL_PRIVATE void* CrashInvestAllocMemory(size_t a_count);
CRASH_INVEST_DLL_PRIVATE void* CrashInvestReallocMemory(void* a_ptr, size_t a_count);
CRASH_INVEST_DLL_PRIVATE void* CrashInvestCallocMemory(size_t a_nmemb, size_t a_size);
CRASH_INVEST_DLL_PRIVATE void  CrashInvestFreeMemory(void* a_ptr);

#define CrashInvestMutexLock(...)
#define CrashInvestMutexUnlock(...)

static bool AllocCalledDefault(const CrashInvestClbkArg* a_data) {
	CRASH_INVEST_STATIC_CAST(void, a_data);
	return true;
}

static void* s_allocClbkData = CRASH_INVEST_NULL;
static CrashInvestHookClbkType s_allocFncCalledClbk = &AllocCalledDefault;
CrashInvestMutex	s_mutexForClbk;
static bool s_bInFunctionCall = false;  // todo: make this thread specific

CRASH_INVEST_DLL_PUBLIC void* malloc(size_t a_count)
{
	CrashInvestClbkArg aArg;

	CrashInvestMutexLock(s_mutexForClbk);
	if (s_bInFunctionCall) {
		CrashInvestMutexUnlock(s_mutexForClbk);
		return CrashInvestAllocMemory(a_count);
	}

	aArg.outMemory = CrashInvestAllocMemory(a_count);
	aArg.clbkData = s_allocClbkData;
	aArg.inpMemory = CRASH_INVEST_NULL;
	aArg.count = a_count;
	aArg.callocOneItemSize = 0;
	aArg.funcType = CrashInvestMalloc;
	(*s_allocFncCalledClbk)(&aArg);
	CrashInvestMutexUnlock(s_mutexForClbk);

	return aArg.outMemory;
}


CRASH_INVEST_DLL_PUBLIC void* realloc(void* a_ptr, size_t a_count)
{
	CrashInvestClbkArg aArg;

	CrashInvestMutexLock(s_mutexForClbk);
	if (s_bInFunctionCall) {
		CrashInvestMutexUnlock(s_mutexForClbk);
		return CrashInvestReallocMemory(a_ptr,a_count);
	}

	aArg.outMemory = CrashInvestReallocMemory(a_ptr,a_count);
	aArg.clbkData = s_allocClbkData;
	aArg.inpMemory = a_ptr;
	aArg.count = a_count;
	aArg.callocOneItemSize = 0;
	aArg.funcType = CrashInvestRealloc;
	(*s_allocFncCalledClbk)(&aArg);
	CrashInvestMutexUnlock(s_mutexForClbk);

	return aArg.outMemory;
}


CRASH_INVEST_DLL_PUBLIC void* calloc(size_t a_nmemb, size_t a_size)
{
	CrashInvestClbkArg aArg;

	CrashInvestMutexLock(s_mutexForClbk);
	if (s_bInFunctionCall) {
		CrashInvestMutexUnlock(s_mutexForClbk);
		return CrashInvestCallocMemory(a_nmemb, a_size);
	}

	aArg.outMemory = CrashInvestCallocMemory(a_nmemb, a_size);
	aArg.clbkData = s_allocClbkData;
	aArg.inpMemory = CRASH_INVEST_NULL;
	aArg.count = a_nmemb;
	aArg.callocOneItemSize = a_size;
	aArg.funcType = CrashInvestCalloc;
	(*s_allocFncCalledClbk)(&aArg);
	CrashInvestMutexUnlock(s_mutexForClbk);

	return aArg.outMemory;
}


CRASH_INVEST_DLL_PUBLIC void free(void* a_ptr)
{
	CrashInvestClbkArg aArg;

	CrashInvestMutexLock(s_mutexForClbk);
	if (s_bInFunctionCall) {
		CrashInvestMutexUnlock(s_mutexForClbk);
		CrashInvestFreeMemory(a_ptr);
		return;
	}

	aArg.outMemory = CRASH_INVEST_NULL;
	aArg.clbkData = s_allocClbkData;
	aArg.inpMemory = a_ptr;
	aArg.count = 0;
	aArg.callocOneItemSize = 0;
	aArg.funcType = CrashInvestFree;
	if ((*s_allocFncCalledClbk)(&aArg)) {
		CrashInvestMutexUnlock(s_mutexForClbk);
		CrashInvestFreeMemory(a_ptr);
	}
}



CRASH_INVEST_HOOK_EXPORT CrashInvestHookClbkType CrashInvestSetNewHookClbk(void* a_clbkData, CrashInvestHookClbkType a_newCallback)
{
	CrashInvestHookClbkType retHook;

	CrashInvestMutexLock(s_mutexForClbk);
	retHook = s_allocFncCalledClbk;
	s_allocClbkData = a_clbkData;
	s_allocFncCalledClbk = a_newCallback ? a_newCallback : (&AllocCalledDefault);
	CrashInvestMutexUnlock(s_mutexForClbk);

	return retHook;
}

CRASH_INVEST_END_C


#endif  // #ifdef _WIN32
