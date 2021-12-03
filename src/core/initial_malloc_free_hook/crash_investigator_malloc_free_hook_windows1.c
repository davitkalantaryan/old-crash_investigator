//
// file:		crash_investigator_malloc_free_hook_windows2.c
// path:		src/core/crash_investigator_malloc_free_hook_windows2.c
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 24
//

#if defined(_WIN32)

#include <crash_investigator/crash_investigator_internal_header.h>
#include <crash_investigator/crash_investigator_malloc_free_hook.h>



CRASH_INVEST_BEGIN_C

extern CRASH_INVEST_DLL_PRIVATE void  free_default(void* a_ptr);
extern CRASH_INVEST_DLL_PRIVATE void* malloc_default(size_t a_count);
extern CRASH_INVEST_DLL_PRIVATE void* realloc_default(void* a_ptr, size_t a_count);
extern CRASH_INVEST_DLL_PRIVATE void* calloc_default(size_t a_nmemb, size_t a_size);


CRASH_INVEST_DLL_PUBLIC TypeMalloc		g_callers_malloc	= &malloc_default;
CRASH_INVEST_DLL_PUBLIC TypeCalloc		g_callers_calloc	= &calloc_default;
CRASH_INVEST_DLL_PUBLIC TypeRealloc		g_callers_realloc	= &realloc_default;
CRASH_INVEST_DLL_PUBLIC TypeFree		g_callers_free		= &free_default;


CRASH_INVEST_DLL_PUBLIC void* malloc(size_t a_count)
{
	return (*g_callers_malloc)(a_count);
}


CRASH_INVEST_DLL_PUBLIC void* calloc(size_t a_nmemb, size_t a_size)
{
	return (*g_callers_calloc)(a_nmemb,a_size);
}


CRASH_INVEST_DLL_PUBLIC void* realloc(void* a_ptr, size_t a_count)
{
	return (*g_callers_realloc)(a_ptr,a_count);
}


CRASH_INVEST_DLL_PUBLIC void free(void* a_ptr)
{
	(*g_callers_free)(a_ptr);
}


CRASH_INVEST_END_C


#endif  // #ifdef _WIN32
