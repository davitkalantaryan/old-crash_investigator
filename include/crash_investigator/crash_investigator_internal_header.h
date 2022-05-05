//
// file:			crash_investigator_internal_header.h
// path:			include/crash_investigator/crash_investigator_internal_header.h
// created on:		2021 Nov 19
// created by:		Davit Kalantaryan (davit.kalantaryan@gmail.com)
//

#ifndef INCLUDE_CRASH_INVEST_INTERNAL_HEADER_H
#define INCLUDE_CRASH_INVEST_INTERNAL_HEADER_H

#include <cpputils_internal_header.h>


#if defined(CRASH_INVEST_COMPILING_SHARED_LIB)
    #define CRASH_INVEST_EXPORT CPPUTILS_DLL_PUBLIC
#elif defined(CRASH_INVEST_USING_STATIC_LIB_OR_OBJECTS)
    #define CRASH_INVEST_EXPORT
#elif defined(CRASH_INVEST_IMPORT_FROM_SHARED_LIB)
    #define CRASH_INVEST_EXPORT CPPUTILS_IMPORT_FROM_DLL
#else
    #define CRASH_INVEST_EXPORT CPPUTILS_DLL_PRIVATE
#endif

#if defined(CRASH_INVEST_HOOK_COMPILING_SHARED_LIB)
    #define CRASH_INVEST_HOOK_EXPORT CPPUTILS_DLL_PUBLIC
#elif defined(CRASH_INVEST_HOOK_USING_STATIC_LIB_OR_OBJECTS)
    #define CRASH_INVEST_HOOK_EXPORT
#elif defined(CRASH_INVEST_HOOK_IMPORT_FROM_SHARED_LIB)
    #define CRASH_INVEST_HOOK_EXPORT CPPUTILS_IMPORT_FROM_DLL
#else
#define CRASH_INVEST_HOOK_EXPORT CPPUTILS_DLL_PRIVATE
#endif

#ifdef _MSC_VER
    #define CRASH_INVEST_ALLOC_EXP  CPPUTILS_DLL_PUBLIC
#else
    #define CRASH_INVEST_ALLOC_EXP  CPPUTILS_DLL_PRIVATE
#endif


#endif  // #ifndef INCLUDE_CRASH_INVEST_INTERNAL_HEADER_H
