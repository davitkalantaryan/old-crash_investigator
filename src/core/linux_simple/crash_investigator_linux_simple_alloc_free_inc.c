//
// file:			crash_investigator_linux_simple_malloc_free.cpp
// path:			src/core/crash_investigator_linux_simple_malloc_free.cpp
// created on:		2023 Mar 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


//#define MEMORY_HANDLE_WAIT_FOR_DEBUGGER

#include <cinternal/internal_header.h>

#if defined(__linux__) || defined(__linux)

#include <crash_investigator/alloc_free.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <dlfcn.h>


#define MEMORY_HANDLER_INIT_MEM_SIZE    16384


CPPUTILS_BEGIN_C

#pragma GCC diagnostic ignored "-Wattributes"

struct CPPUTILS_DLL_PRIVATE SMemoryHandlerInitMemData{
    size_t      totalSize;
    char        reserved[16-sizeof(size_t)];
};

static void* MemoryHandlerMallocInitialStatic(size_t a_size);
static void* MemoryHandlerCallocInitialStatic(size_t a_nmemb, size_t a_size);
static void* MemoryHandlerReallocInitialStatic(void* a_ptr, size_t a_size);
static void  MemoryHandlerFreeInitialStatic(void* a_ptr);

static int s_nStartedInitLibrary = 0;
CPPUTILS_DLL_PRIVATE TypeMemoryHandlerMalloc  g_malloc  = CPPUTILS_NULL;
CPPUTILS_DLL_PRIVATE TypeMemoryHandlerCalloc  g_calloc  = CPPUTILS_NULL;
CPPUTILS_DLL_PRIVATE TypeMemoryHandlerRealloc g_realloc = CPPUTILS_NULL;
CPPUTILS_DLL_PRIVATE TypeMemoryHandlerFree    g_free    = CPPUTILS_NULL;

static TypeMemoryHandlerMalloc  s_malloc_c_lib  = CPPUTILS_NULL;
static TypeMemoryHandlerCalloc  s_calloc_c_lib  = CPPUTILS_NULL;
static TypeMemoryHandlerRealloc s_realloc_c_lib = CPPUTILS_NULL;
static TypeMemoryHandlerFree    s_free_c_lib    = CPPUTILS_NULL;

static size_t   s_unInitialMemoryOffset = 0;
static char     s_vcInitialBuffer[MEMORY_HANDLER_INIT_MEM_SIZE];


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

MEM_HANDLE_EXPORT void* MemoryHandlerCLibMalloc(size_t a_size)
{
    return (*s_malloc_c_lib)(a_size);
}


MEM_HANDLE_EXPORT void* MemoryHandlerCLibCalloc(size_t a_nmemb, size_t a_size)
{
    return (*s_calloc_c_lib)(a_nmemb,a_size);
}


MEM_HANDLE_EXPORT void* MemoryHandlerCLibRealloc(void* a_ptr, size_t a_size)
{
    return (*s_realloc_c_lib)(a_ptr,a_size);
}


MEM_HANDLE_EXPORT void MemoryHandlerCLibFree(void* a_ptr)
{
    (*s_free_c_lib)(a_ptr);
}


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/


static inline size_t MemoryHandlerCalculateRoundedMemorySizeInline(size_t a_initialSize){
    a_initialSize += sizeof(struct SMemoryHandlerInitMemData);
    if(0x8 & a_initialSize){
        return ((a_initialSize>>3)+1)<<3;
    }
    return a_initialSize;
}


static inline void InitLibraryInline(void){
    if(!s_nStartedInitLibrary){
        // let's backup the values.
        // maybe some other piece of software changed the values of the pointers
        // we should remember and set them back
        const TypeMemoryHandlerMalloc  aMalloc = g_malloc;
        const TypeMemoryHandlerCalloc  aCalloc = g_calloc;
        const TypeMemoryHandlerRealloc aRealloc = g_realloc;
        const TypeMemoryHandlerFree    aFree = g_free;

        s_nStartedInitLibrary = 1;

        // let's make sure that the pointers are initial
        g_malloc  = &MemoryHandlerMallocInitialStatic;
        g_calloc  = &MemoryHandlerCallocInitialStatic;
        g_realloc = &MemoryHandlerReallocInitialStatic;
        g_free    = &MemoryHandlerFreeInitialStatic;

        s_malloc_c_lib  = (TypeMemoryHandlerMalloc)dlsym(RTLD_NEXT, "malloc");
        s_calloc_c_lib  = (TypeMemoryHandlerCalloc)dlsym(RTLD_NEXT, "calloc");
        s_realloc_c_lib = (TypeMemoryHandlerRealloc)dlsym(RTLD_NEXT, "realloc");
        s_free_c_lib    = (TypeMemoryHandlerFree)dlsym(RTLD_NEXT, "free");
        if((!s_malloc_c_lib)||(!s_calloc_c_lib)||(!s_realloc_c_lib)||(!s_free_c_lib)){
            //fprintf(stderr, "Unable to get addresses of original functions (malloc/realloc/free)\n. Application will exit");
            //fflush(stderr);
            exit(1);
        }

        g_malloc  = aMalloc?aMalloc:s_malloc_c_lib;
        g_calloc  = aCalloc?aCalloc:s_calloc_c_lib;
        g_realloc = aRealloc?aRealloc:s_realloc_c_lib;
        g_free    = aFree?aFree:s_free_c_lib;

#ifdef MEMORY_HANDLE_WAIT_FOR_DEBUGGER
        fprintf(stdout,"Press any key then press enter to continue "); fflush(stdout);
        getchar();
#endif
    }
}


static void* MemoryHandlerMallocInitialStatic(size_t a_size)
{
    InitLibraryInline();
    if(a_size){
        const size_t cunTotalSize = MemoryHandlerCalculateRoundedMemorySizeInline(a_size);
        const size_t cunNewOffset = s_unInitialMemoryOffset + cunTotalSize;
        if(cunNewOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
            char*const pcCurrentMemPointer = s_vcInitialBuffer + s_unInitialMemoryOffset;
            struct SMemoryHandlerInitMemData*const pItem = (struct SMemoryHandlerInitMemData*)pcCurrentMemPointer;
            pItem->totalSize = cunTotalSize;
            return pcCurrentMemPointer + sizeof(struct SMemoryHandlerInitMemData);
        }
    }

    return CPPUTILS_NULL;
}


static void* MemoryHandlerCallocInitialStatic(size_t a_nmemb, size_t a_size2)
{
    const size_t cunTotalWantedByUser = a_nmemb * a_size2;
    InitLibraryInline();
    if(cunTotalWantedByUser){
        const size_t cunTotalSize = MemoryHandlerCalculateRoundedMemorySizeInline(cunTotalWantedByUser);
        const size_t cunNewOffset = s_unInitialMemoryOffset + cunTotalSize;
        if(cunNewOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
            char* pReturnPointer;
            char*const pcCurrentMemPointer = s_vcInitialBuffer + s_unInitialMemoryOffset;
            struct SMemoryHandlerInitMemData*const pItem = (struct SMemoryHandlerInitMemData*)pcCurrentMemPointer;
            pItem->totalSize = cunTotalSize;
            pReturnPointer = pcCurrentMemPointer + sizeof(struct SMemoryHandlerInitMemData);
            memset(pReturnPointer,0,cunTotalWantedByUser);
            return pReturnPointer;
        }

    }
    return CPPUTILS_NULL;
}


static inline size_t MemoryHandlerCalculateExistingMemOffsetInline(void* a_ptr){
    const size_t cunOffsetInit = (size_t)(((char*)a_ptr)-s_vcInitialBuffer);
    return cunOffsetInit - sizeof(struct SMemoryHandlerInitMemData);
}


static void* MemoryHandlerReallocInitialStatic(void* a_ptr, size_t a_size)
{
    InitLibraryInline();
    if(a_ptr){
        const size_t cunOffset = MemoryHandlerCalculateExistingMemOffsetInline(a_ptr);
        char*const pcCurrentMemPointer = s_vcInitialBuffer + cunOffset;
        struct SMemoryHandlerInitMemData*const pItem = (struct SMemoryHandlerInitMemData*)pcCurrentMemPointer;
        if(((pItem->totalSize)+cunOffset)==s_unInitialMemoryOffset){
            // this is a last element, simply increase it, or decrease
            const size_t cunTotalSize = a_size?MemoryHandlerCalculateRoundedMemorySizeInline(a_size):0;
            const size_t cunNewOffset = cunOffset+cunTotalSize;
            if(cunNewOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
                pItem->totalSize = cunTotalSize;
                s_unInitialMemoryOffset = cunNewOffset;
                return a_ptr;
            }

            return CPPUTILS_NULL;
        }

        // in case if (((pItem->totalSize)+cunOffset)!=s_unInitialMemoryOffset) we should forgot about existing memory and allocate new one
    }

    return MemoryHandlerMallocInitialStatic(a_size);
}


static void MemoryHandlerFreeInitialStatic(void* a_ptr)
{
    InitLibraryInline();
    if(a_ptr){
        const size_t cunOffset = MemoryHandlerCalculateExistingMemOffsetInline(a_ptr);
        const char*const pcCurrentMemPointer = s_vcInitialBuffer + cunOffset;
        const struct SMemoryHandlerInitMemData* pItem = (const struct SMemoryHandlerInitMemData*)pcCurrentMemPointer;
        if(((pItem->totalSize)+cunOffset)==s_unInitialMemoryOffset){
            // this is a last element, simply increase it, or decrease
            s_unInitialMemoryOffset = cunOffset;
        }

        // in case if (((pItem->totalSize)+cunOffset)!=s_unInitialMemoryOffset) we should forgot about existing memory
    }
}



CPPUTILS_DLL_PUBLIC void* MemoryHandlerRealloc(void* a_ptr, size_t a_size)
{
    if(a_ptr){
        const size_t cunMemPosition = (size_t)((char*)a_ptr);
        const size_t cunInitMemPosition = (size_t)s_vcInitialBuffer;
        if(cunMemPosition>cunInitMemPosition){
            size_t cunOffset = cunMemPosition-cunInitMemPosition;
            if(cunOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
                // the speech is about freeing iniytial memory. The initial memory is not in use, so simply forgot it
                if(a_size>0){
                    void* pRet;
                    size_t memcpySize;
                    char* pcCurrentMemPointer;
                    struct SMemoryHandlerInitMemData* pItem;
                    cunOffset -= sizeof(struct SMemoryHandlerInitMemData);
                    pcCurrentMemPointer = s_vcInitialBuffer + cunOffset;
                    pItem = (struct SMemoryHandlerInitMemData*)pcCurrentMemPointer;
                    pRet = (*g_realloc)(CPPUTILS_NULL,a_size);
                    memcpySize = (a_size<pItem->totalSize)?a_size:(pItem->totalSize);
                    memcpy(pRet,pcCurrentMemPointer+sizeof(struct SMemoryHandlerInitMemData),memcpySize);
                    return pRet;
                }
                else{
                    return CPPUTILS_NULL;
                }
            }  //  if(cunOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
        }  //  if(cunMemPosition>cunInitMemPosition){
    }  //  if(a_ptr){

    return (*g_realloc)(a_ptr,a_size);  // in all other cases call the users callback
}


CPPUTILS_DLL_PRIVATE void MemoryHandlerFree(void* a_ptr)
{
    if(a_ptr){
        const size_t cunMemPosition = (size_t)((char*)a_ptr);
        const size_t cunInitMemPosition = (size_t)s_vcInitialBuffer;
        if(cunMemPosition>cunInitMemPosition){
            const size_t cunOffset = cunMemPosition-cunInitMemPosition;
            if(cunOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
                // the speech is about freeing iniytial memory. The initial memory is not in use, so simply forgot it
                return;
            }  //  if(cunOffset<MEMORY_HANDLER_INIT_MEM_SIZE){
        }  //  if(cunMemPosition>cunInitMemPosition){
    }  //  if(a_ptr){

    (*g_free)(a_ptr);  // in all other cases call the users callback
}


//
CPPUTILS_CODE_INITIALIZER(MemoryHandlerInit){
    InitLibraryInline();
}



CPPUTILS_END_C


#endif  //  #if defined(__linux__) || defined(__linux)
