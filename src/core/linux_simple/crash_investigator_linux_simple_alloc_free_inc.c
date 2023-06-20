//
// file:			crash_investigator_linux_simple_malloc_free.cpp
// path:			src/core/crash_investigator_linux_simple_malloc_free.cpp
// created on:		2023 Mar 06
// created by:		Davit Kalantaryan (davit.kalantaryan@desy.de)
//


//#define MEMORY_HANDLE_WAIT_FOR_DEBUGGER
//#define ANALIZE_ALLOC_FREE_COUNT    1

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
#ifdef ANALIZE_ALLOC_FREE_COUNT
#include <crash_investigator/analyze_leaking.h>
#endif


#define MEMORY_HANDLER_INIT_MEM_SIZE    16384


CPPUTILS_BEGIN_C

#pragma GCC diagnostic ignored "-Wattributes"

static int s_nLibraryInited = 0;


#ifdef ANALIZE_ALLOC_FREE_COUNT

static __thread int s_nIgnoreForThisThread = 0;
static struct SCrInvAnalyzeLeakingData   s_analyzeData;

#endif  //  #ifdef ANALIZE_ALLOC_FREE_COUNT

struct CPPUTILS_DLL_PRIVATE SMemoryHandlerInitMemData{
    size_t      totalSize;
    char        reserved[16-sizeof(size_t)];
};


static int s_nStartedInitLibrary = 0;

static void* MemoryHandlerMallocInitialStatic(size_t a_size);
static void* MemoryHandlerCallocInitialStatic(size_t a_nmemb, size_t a_size);
static void* MemoryHandlerReallocInitialStatic(void* a_ptr, size_t a_size);
static void  MemoryHandlerFreeInitialStatic(void* a_ptr);

CPPUTILS_DLL_PRIVATE TypeMemoryHandlerMalloc  g_malloc  = &MemoryHandlerMallocInitialStatic;
CPPUTILS_DLL_PRIVATE TypeMemoryHandlerCalloc  g_calloc  = &MemoryHandlerCallocInitialStatic;
CPPUTILS_DLL_PRIVATE TypeMemoryHandlerRealloc g_realloc = &MemoryHandlerReallocInitialStatic;
CPPUTILS_DLL_PRIVATE TypeMemoryHandlerFree    g_free    = &MemoryHandlerFreeInitialStatic;

static TypeMemoryHandlerMalloc  s_malloc_tmp  = CPPUTILS_NULL;
static TypeMemoryHandlerCalloc  s_calloc_tmp  = CPPUTILS_NULL;
static TypeMemoryHandlerRealloc s_realloc_tmp = CPPUTILS_NULL;
static TypeMemoryHandlerFree    s_free_tmp    = CPPUTILS_NULL;

static TypeMemoryHandlerMalloc  s_malloc_c_lib  = CPPUTILS_NULL;
static TypeMemoryHandlerCalloc  s_calloc_c_lib  = CPPUTILS_NULL;
static TypeMemoryHandlerRealloc s_realloc_c_lib = CPPUTILS_NULL;
static TypeMemoryHandlerFree    s_free_c_lib    = CPPUTILS_NULL;

#ifdef MEM_HANDLER_MMAP_NEEDED
static void* MemoryHandlerMmapInitialStatic(void* a_addr, size_t a_len, int a_prot, int a_flags,int a_fildes, off_t a_off);
CPPUTILS_DLL_PRIVATE TypeMemoryHandlerMmap  g_mmap  = &MemoryHandlerMmapInitialStatic;
static TypeMemoryHandlerMmap    s_mmap_tmp  = CPPUTILS_NULL;
static TypeMemoryHandlerMmap    s_mmap_c_lib = CPPUTILS_NULL;
#endif

static size_t   s_unInitialMemoryOffset = 0;
static char     s_vcInitialBuffer[MEMORY_HANDLER_INIT_MEM_SIZE];


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/


MEM_HANDLE_EXPORT void MemoryHandlerSetMallocFnc(TypeMemoryHandlerMalloc a_malloc)
{
    if(s_nLibraryInited){
        g_malloc = a_malloc;
    }
    else{
        s_malloc_tmp = a_malloc;
    }
}


MEM_HANDLE_EXPORT void MemoryHandlerSetCallocFnc(TypeMemoryHandlerCalloc a_calloc)
{
    if(s_nLibraryInited){
        g_calloc = a_calloc;
    }
    else{
        s_calloc_tmp = a_calloc;
    }
}


MEM_HANDLE_EXPORT void MemoryHandlerSetReallocFnc(TypeMemoryHandlerRealloc a_realloc)
{
    if(s_nLibraryInited){
        g_realloc = a_realloc;
    }
    else{
        s_realloc_tmp = a_realloc;
    }
}


MEM_HANDLE_EXPORT void MemoryHandlerSetFreeFnc(TypeMemoryHandlerFree a_free)
{
    if(s_nLibraryInited){
        g_free = a_free;
    }
    else{
        s_free_tmp = a_free;
    }
}


#ifdef MEM_HANDLER_MMAP_NEEDED

MEM_HANDLE_EXPORT void MemoryHandlerSetMmapFnc(TypeMemoryHandlerMmap a_mmap)
{
    if(s_nLibraryInited){
        g_mmap = a_mmap;
    }
    else{
        s_mmap_tmp = a_mmap;
    }
}

#endif  //  #ifdef MEM_HANDLER_MMAP_NEEDED


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

MEM_HANDLE_EXPORT void* MemoryHandlerCLibMalloc(size_t a_size)
{
#ifdef ANALIZE_ALLOC_FREE_COUNT
    void*const pRet = (*s_malloc_c_lib)(a_size);
    if(pRet){
        CrashInvestAnalyzeLeakingAddAllocedItem(1,pRet,&s_analyzeData);
    }
    return pRet;
#else
    return (*s_malloc_c_lib)(a_size);
#endif
}


MEM_HANDLE_EXPORT void* MemoryHandlerCLibCalloc(size_t a_nmemb, size_t a_size)
{
#ifdef ANALIZE_ALLOC_FREE_COUNT
    void*const pRet = (*s_calloc_c_lib)(a_nmemb,a_size);
    if(pRet){
        CrashInvestAnalyzeLeakingAddAllocedItem(1,pRet,&s_analyzeData);
    }
    return pRet;
#else
    return (*s_calloc_c_lib)(a_nmemb,a_size);    
#endif
}


MEM_HANDLE_EXPORT void* MemoryHandlerCLibRealloc(void* a_ptr, size_t a_size)
{
#ifdef ANALIZE_ALLOC_FREE_COUNT
    void*const pRet = MemoryHandlerCLibRealloc(a_ptr,a_size);
    if(a_size){
        if(pRet && (pRet!=a_ptr)){
            CrashInvestAnalyzeLeakingRemoveAllocedItem(a_ptr,&s_analyzeData);
            CrashInvestAnalyzeLeakingAddAllocedItem(1,pRet,&s_analyzeData);
        }
        return pRet;
    }
    else{
        CrashInvestAnalyzeLeakingRemoveAllocedItem(a_ptr,&s_analyzeData);
    }

    return pRet;
#else
    return (*s_realloc_c_lib)(a_ptr,a_size);    
#endif
}


MEM_HANDLE_EXPORT void MemoryHandlerCLibFree(void* a_ptr)
{
#ifdef ANALIZE_ALLOC_FREE_COUNT
    if(a_ptr){
        CrashInvestAnalyzeLeakingRemoveAllocedItem(a_ptr,&s_analyzeData);
    }
#endif
    (*s_free_c_lib)(a_ptr);
    //(*s_free_c_lib)(a_ptr);
}


#ifdef MEM_HANDLER_MMAP_NEEDED


MEM_HANDLE_EXPORT void* MemoryHandlerCLibMmap(void* a_addr, size_t a_len, int a_prot, int a_flags,int a_fildes, off_t a_off)
{
    return (*s_mmap_c_lib)(a_addr,a_len,a_prot,a_flags,a_fildes,a_off);
}

#endif  //  #ifdef MEM_HANDLER_MMAP_NEEDED


/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////*/


static inline size_t MemoryHandlerCalculateRoundedMemorySizeInline(size_t a_initialSize){
    a_initialSize += sizeof(struct SMemoryHandlerInitMemData);
    if(0x8 & a_initialSize){
        return ((a_initialSize>>3)+1)<<3;
    }
    return a_initialSize;
}


static void crash_investigator_linux_simple_alloc_free_inc_clean(void){

    g_malloc  = s_malloc_c_lib;
    g_calloc  = s_calloc_c_lib;
    g_realloc = s_realloc_c_lib;
    g_free    = s_free_c_lib;

#ifdef ANALIZE_ALLOC_FREE_COUNT
    CrashInvestAnalyzeLeakingClean(&s_analyzeData);
#endif

}


static inline void InitLibraryInline(void){
    if(!s_nStartedInitLibrary){

        s_nStartedInitLibrary = 1;

        s_malloc_c_lib  = (TypeMemoryHandlerMalloc)dlsym(RTLD_NEXT, "malloc");
        s_calloc_c_lib  = (TypeMemoryHandlerCalloc)dlsym(RTLD_NEXT, "calloc");
        s_realloc_c_lib = (TypeMemoryHandlerRealloc)dlsym(RTLD_NEXT, "realloc");
        s_free_c_lib    = (TypeMemoryHandlerFree)dlsym(RTLD_NEXT, "free");
        if((!s_malloc_c_lib)||(!s_calloc_c_lib)||(!s_realloc_c_lib)||(!s_free_c_lib)){
            //fprintf(stderr, "Unable to get addresses of original functions (malloc/realloc/free)\n. Application will exit");
            //fflush(stderr);
            exit(1);
        }

#ifdef MEM_HANDLER_MMAP_NEEDED

        s_mmap_c_lib    = (TypeMemoryHandlerMmap)dlsym(RTLD_NEXT, "mmap");
        if(!s_mmap_c_lib){
            //fprintf(stderr, "Unable to get addresses of original functions (malloc/realloc/free)\n. Application will exit");
            //fflush(stderr);
            exit(1);
        }
        g_mmap  = s_mmap_tmp?s_mmap_tmp:s_mmap_c_lib;

#endif  //  #ifdef MEM_HANDLER_MMAP_NEEDED

        g_malloc  = s_malloc_tmp?s_malloc_tmp:s_malloc_c_lib;
        g_calloc  = s_calloc_tmp?s_calloc_tmp:s_calloc_c_lib;
        g_realloc = s_realloc_tmp?s_realloc_tmp:s_realloc_c_lib;
        g_free    = s_free_tmp?s_free_tmp:s_free_c_lib;

#ifdef ANALIZE_ALLOC_FREE_COUNT

        if(CrashInvestAnalyzeLeakingInitialize(&s_analyzeData,&s_nIgnoreForThisThread,"MEMORY_LEAK_ANALYZE_INIT_TIME_SEC_DEFAULT","MEMORY_LEAK_ANALYZE_MAX_ALLOC_DEFAULT")){
            g_malloc  = s_malloc_c_lib;
            g_calloc  = s_calloc_c_lib;
            g_realloc = s_realloc_c_lib;
            g_free    = s_free_c_lib;
            exit(1);
        }

#endif  //  #ifdef ANALIZE_ALLOC_FREE_COUNT

        s_nLibraryInited = 1;

        atexit(&crash_investigator_linux_simple_alloc_free_inc_clean);

#ifdef MEMORY_HANDLE_WAIT_FOR_DEBUGGER
        fprintf(stdout,"Press any key then press enter to continue "); fflush(stdout);
        getchar();
#endif

    }  //  if(!s_nStartedInitLibrary){
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


#ifdef MEM_HANDLER_MMAP_NEEDED

static void* MemoryHandlerMmapInitialStatic(void* a_addr, size_t a_size, int a_prot, int a_flags,int a_fildes, off_t a_off)
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

        CPPUTILS_STATIC_CAST(void,a_addr);
        CPPUTILS_STATIC_CAST(void,a_prot);
        CPPUTILS_STATIC_CAST(void,a_flags);
        CPPUTILS_STATIC_CAST(void,a_fildes);
        CPPUTILS_STATIC_CAST(void,a_off);
    }

    return CPPUTILS_NULL;
}

#endif  //  #ifdef MEM_HANDLER_MMAP_NEEDED



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
CPPUTILS_C_CODE_INITIALIZER(MemoryHandlerInit){
    InitLibraryInline();
}



CPPUTILS_END_C


#endif  //  #if defined(__linux__) || defined(__linux)
