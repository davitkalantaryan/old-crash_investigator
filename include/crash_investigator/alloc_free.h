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
#ifdef MEM_HANDLER_MMAP_NEEDED
#include <bits/types.h>
#endif


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


#ifdef MEM_HANDLER_MMAP_NEEDED
#ifndef __off_t_defined
# ifndef __USE_FILE_OFFSET64
typedef __off_t off_t;
# else
typedef __off64_t off_t;
# endif
# define __off_t_defined
#endif
typedef void* (*TypeMemoryHandlerMmap)(void*,size_t,int,int,int,off_t);
MEM_HANDLE_EXPORT void MemoryHandlerSetMmapFnc(TypeMemoryHandlerMmap a_mmap);
MEM_HANDLE_EXPORT void* MemoryHandlerCLibMmap(void *addr, size_t len, int prot, int flags,int fildes, off_t off);
#endif  //  #ifdef MEM_HANDLER_MMAP_NEEDED



CPPUTILS_END_C


#endif  //  #ifndef CRASH_INVESTIGATOR_INCLUDE_CRASH_INVESTIGATOR_ALLOC_FREE_H
