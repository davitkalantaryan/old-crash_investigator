//
// file:			crash_investigator_malloc_free_hook.h
// path:			include/crash_investigator/crash_investigator_malloc_free_hook.h
// created on:		2021 Nov 24
// created by:		Davit Kalantaryan (davit.kalantaryan@gmail.com)
//

#ifndef SRC_INCLUDE_CRASH_INVEST_MALLOC_FREE_HOOK_H
#define SRC_INCLUDE_CRASH_INVEST_MALLOC_FREE_HOOK_H

#include <crash_investigator/crash_investigator_internal_header.h>
#include <stddef.h>
#include <stdbool.h>

CRASH_INVEST_BEGIN_C

typedef enum {
	CrashInvestMalloc,
	CrashInvestRealloc,
	CrashInvestCalloc,
	CrashInvestFree
}CrashInvestFnc;

typedef struct {
	void* clbkData;
	void* inpMemory;
	void* outMemory;
	size_t count;
	size_t callocOneItemSize;
	CrashInvestFnc	funcType;
//#if sizeof(CrashInvestFnc)!=sizeof(size_t)
//	int    reservedö
//#endif
}CrashInvestClbkArg;

typedef bool (*CrashInvestHookClbkType)(const CrashInvestClbkArg* data);

CRASH_INVEST_HOOK_EXPORT CrashInvestHookClbkType CrashInvestSetNewHookClbk(void* clbkData, CrashInvestHookClbkType newCallback);

CRASH_INVEST_END_C


#endif  // #ifndef SRC_INCLUDE_CRASH_INVEST_MALLOC_FREE_HOOK_H
