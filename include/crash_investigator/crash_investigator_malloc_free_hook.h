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


CPPUTILS_BEGIN_C

typedef void* (*TypeMalloc)(size_t);
typedef void* (*TypeCalloc)(size_t a_nmemb, size_t a_size);
typedef void* (*TypeRealloc)(void*, size_t);
typedef void  (*TypeFree)(void*);

struct SCrInvAllFreeFunctions {
	TypeFree		m_free;
	TypeMalloc		m_malloc;
	TypeCalloc		m_calloc;
	TypeRealloc		m_realloc;
};

extern CRASH_INVEST_HOOK_EXPORT struct SCrInvAllFreeFunctions* s_pCrInvAllocFncsPtrs;


CRASH_INVEST_HOOK_EXPORT void CrInvAllocFreeInitializationRoutine(void);
CRASH_INVEST_HOOK_EXPORT void CrInvAllocFreeCleanupRoutine(void);


CPPUTILS_END_C


#endif  // #ifndef SRC_INCLUDE_CRASH_INVEST_MALLOC_FREE_HOOK_H
