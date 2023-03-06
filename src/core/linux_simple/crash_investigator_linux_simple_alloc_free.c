//
// file:			crash_investigator_linux_simple_malloc_free.cpp
// path:			src/core/crash_investigator_linux_simple_malloc_free.cpp
// created on:		2023 Mar 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


#include <cinternal/internal_header.h>

#if defined(__linux__) || defined(__linux)

#include <crash_investigator/alloc_free.h>



CPPUTILS_BEGIN_C

extern CPPUTILS_DLL_PRIVATE TypeMemoryHandlerMalloc  g_malloc;
extern CPPUTILS_DLL_PRIVATE TypeMemoryHandlerCalloc  g_calloc;
extern CPPUTILS_DLL_PRIVATE TypeMemoryHandlerRealloc g_realloc;
extern CPPUTILS_DLL_PRIVATE TypeMemoryHandlerFree    g_free ;

CPPUTILS_DLL_PUBLIC void* MemoryHandlerRealloc(void* a_ptr, size_t a_size);
CPPUTILS_DLL_PRIVATE void MemoryHandlerFree(void* a_ptr);


CPPUTILS_DLL_PUBLIC void* malloc(size_t a_size)
{
    return (*g_malloc)(a_size);
}


CPPUTILS_DLL_PUBLIC void* calloc(size_t a_nmemb, size_t a_size)
{
    return (*g_calloc)(a_nmemb,a_size);
}


CPPUTILS_DLL_PUBLIC void* realloc(void* a_ptr, size_t a_size)
{
    return MemoryHandlerRealloc(a_ptr,a_size);
}


CPPUTILS_DLL_PUBLIC void free(void* a_ptr)
{
    MemoryHandlerFree(a_ptr);
}



CPPUTILS_END_C


#endif  //  #if defined(__linux__) || defined(__linux)
