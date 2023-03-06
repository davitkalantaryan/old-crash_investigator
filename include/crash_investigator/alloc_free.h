//
// file:			alloc_free.h
// path:			include/crash_investigator/alloc_free.h
// created on:		2023 Mar 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//

#ifndef CRASH_INVESTIGATOR_INCLUDE_CRASH_INVESTIGATOR_ALLOC_FREE_H
#define CRASH_INVESTIGATOR_INCLUDE_CRASH_INVESTIGATOR_ALLOC_FREE_H


#include <crash_investigator/crash_investigator_internal_header.h>
#include <stddef.h>


CPPUTILS_BEGIN_C


typedef void* (*TypeMemoryHandlerMalloc)(size_t);
typedef void* (*TypeMemoryHandlerCalloc)(size_t, size_t);
typedef void* (*TypeMemoryHandlerRealloc)(void*,size_t);
typedef void  (*TypeMemoryHandlerFree)(void*);


MEM_HANDLE_EXPORT void MemoryHandlerSetMallocFnc(TypeMemoryHandlerMalloc a_malloc);
MEM_HANDLE_EXPORT void MemoryHandlerSetCallocFnc(TypeMemoryHandlerCalloc a_calloc);
MEM_HANDLE_EXPORT void MemoryHandlerSetReallocFnc(TypeMemoryHandlerRealloc a_realloc);
MEM_HANDLE_EXPORT void MemoryHandlerSetFreeFnc(TypeMemoryHandlerFree a_free);


MEM_HANDLE_EXPORT void* MemoryHandlerCLibMalloc(size_t a_size);
MEM_HANDLE_EXPORT void* MemoryHandlerCLibCalloc(size_t a_nmemb, size_t a_size);
MEM_HANDLE_EXPORT void* MemoryHandlerCLibRealloc(void* a_ptr, size_t a_size);
MEM_HANDLE_EXPORT void  MemoryHandlerCLibFree(void* a_ptr);


CPPUTILS_END_C


#endif  //  #ifndef CRASH_INVESTIGATOR_INCLUDE_CRASH_INVESTIGATOR_ALLOC_FREE_H
