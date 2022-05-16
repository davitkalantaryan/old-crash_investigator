//
// file:		crash_investigator_malloc_free_hook_windows2.c
// path:		src/core/crash_investigator_malloc_free_hook_windows2.c
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 24
//

#if defined(_WIN32)

#include <crash_investigator/crash_investigator_internal_header.h>
#include <crash_investigator/crash_investigator_malloc_free_hook.h>



CPPUTILS_BEGIN_C


extern CPPUTILS_DLL_PUBLIC struct SCrInvAllFreeFunctions* s_pCrInvAllocFncsPtrs;


CPPUTILS_DLL_PUBLIC void free(void* a_ptr)
{
	(*s_pCrInvAllocFncsPtrs->m_free)(a_ptr);
}


CPPUTILS_DLL_PUBLIC void* malloc(size_t a_count)
{
	return (*s_pCrInvAllocFncsPtrs->m_malloc)(a_count);
}


CPPUTILS_DLL_PUBLIC void* calloc(size_t a_nmemb, size_t a_size)
{
	return (*s_pCrInvAllocFncsPtrs->m_calloc)(a_nmemb,a_size);
}


CPPUTILS_DLL_PUBLIC void* realloc(void* a_ptr, size_t a_count)
{
	return (*s_pCrInvAllocFncsPtrs->m_realloc)(a_ptr,a_count);
}



CPPUTILS_END_C


#endif  // #ifdef _WIN32
