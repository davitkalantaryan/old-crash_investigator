//
// file:			crash_investigator_internal_header.h
// path:			include/crash_investigator/crash_investigator_internal_header.h
// created on:		2021 Nov 19
// created by:		Davit Kalantaryan (davit.kalantaryan@gmail.com)
//

#ifndef INCLUDE_CRASH_INVEST_INTERNAL_HEADER_H
#define INCLUDE_CRASH_INVEST_INTERNAL_HEADER_H

#ifdef CPPUTILS_DEFS_NEEDED
#include <cpputils_internal_header.h>
#endif

#if defined(__EMSCRIPTEN__) || defined(EMSCRIPTEN) || defined(CRASH_INVEST_WASM)
#define CRASH_INVEST_EMSCRIPTEN_IS_USED
#endif

#ifdef SYSTEM_EXE_START_IS_POSSIBLE
#define CRASH_INVEST_EXE_START_IS_POSSIBLE
#endif

#if !defined(CRASH_INVEST_EMSCRIPTEN_IS_USED) // todo: continue list
#define CRASH_INVEST_DESKTOP_IS_USED
#endif

#if !defined(CRASH_INVEST_EMSCRIPTEN_IS_USED) || defined(USE_DLOPEN_FROM_WASM)
#define CRASH_INVEST_DLOPEN_IS_POSSIBLE
#endif

#ifdef _MSC_VER
	#pragma warning (disable:4355)
    #define CRASH_INVEST_BEFORE_CPP_17_FALL_THR
	#if defined(_MSVC_LANG) && (_MSVC_LANG>=201100L)
        #define CRASH_INVEST_CPP_11_DEFINED		1
	#endif
	#if defined(_MSVC_LANG) && (_MSVC_LANG>=201400L)
        #define CRASH_INVEST_CPP_14_DEFINED		1
	#endif
	#if defined(_MSVC_LANG) && (_MSVC_LANG>=201700L)
        #define CRASH_INVEST_CPP_17_DEFINED		1
	#endif
	#if defined(_MSVC_LANG) && (_MSVC_LANG>=202000L)
		#define CRASH_INVEST_CPP_20_DEFINED		1
	#endif
	#define CRASH_INVEST_THISCALL	__thiscall
    #define CRASH_INVEST_DLL_PUBLIC		__declspec(dllexport)
    #define CRASH_INVEST_DLL_PRIVATE
    #define CRASH_INVEST_IMPORT_FROM_DLL	__declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
    #define CRASH_INVEST_BEFORE_CPP_17_FALL_THR	__attribute__ ((fallthrough)) ;
	#if defined(__cplusplus) && (__cplusplus>=201100L)
        #define CRASH_INVEST_CPP_11_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=201400L)
        #define CRASH_INVEST_CPP_14_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=201700L)
        #define CRASH_INVEST_CPP_17_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=202000L)
		#define CRASH_INVEST_CPP_20_DEFINED		1
	#endif
	#define CRASH_INVEST_THISCALL
    //#define CRASH_INVEST_DLL_PUBLIC		__attribute__((visibility("default")))
    #define CRASH_INVEST_DLL_PUBLIC
    #define CRASH_INVEST_DLL_PRIVATE		__attribute__((visibility("hidden")))
    #define CRASH_INVEST_IMPORT_FROM_DLL
#elif defined(__CYGWIN__)
    #define CRASH_INVEST_BEFORE_CPP_17_FALL_THR	__attribute__ ((fallthrough)) ;
	#if defined(__cplusplus) && (__cplusplus>=201100L)
        #define CRASH_INVEST_CPP_11_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=201400L)
        #define CRASH_INVEST_CPP_14_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=201700L)
        #define CRASH_INVEST_CPP_17_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=202000L)
		#define CRASH_INVEST_CPP_20_DEFINED		1
	#endif
	#define CRASH_INVEST_THISCALL
    #define CRASH_INVEST_DLL_PUBLIC		__attribute__((dllexport))
    #define CRASH_INVEST_DLL_PRIVATE
    #define CRASH_INVEST_IMPORT_FROM_DLL	__attribute__((dllimport))
#elif defined(__MINGW64__) || defined(__MINGW32__)
    #define CRASH_INVEST_BEFORE_CPP_17_FALL_THR	__attribute__ ((fallthrough)) ;
	#if defined(__cplusplus) && (__cplusplus>=201100L)
        #define CRASH_INVEST_CPP_11_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=201400L)
        #define CRASH_INVEST_CPP_14_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=201700L)
        #define CRASH_INVEST_CPP_17_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=202000L)
		#define CRASH_INVEST_CPP_20_DEFINED		1
	#endif
	#define CRASH_INVEST_THISCALL
    #define CRASH_INVEST_DLL_PUBLIC		_declspec(dllexport)
    #define CRASH_INVEST_DLL_PRIVATE
    #define CRASH_INVEST_IMPORT_FROM_DLL	_declspec(dllimport)
#elif defined(__SUNPRO_CC)
    // #define CRASH_INVEST_BEFORE_CPP_17_FALL_THR	__attribute__ ((fallthrough)) ; // ???
    #define CRASH_INVEST_BEFORE_CPP_17_FALL_THR
	#if defined(__cplusplus) && (__cplusplus>=201100L)
        #define CRASH_INVEST_CPP_11_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=201400L)
        #define CRASH_INVEST_CPP_14_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=201700L)
        #define CRASH_INVEST_CPP_17_DEFINED		1
	#endif
	#if defined(__cplusplus) && (__cplusplus>=202000L)
		#define CRASH_INVEST_CPP_20_DEFINED		1
	#endif
	#define CRASH_INVEST_THISCALL
    #define CRASH_INVEST_DLL_PUBLIC
    #define CRASH_INVEST_DLL_PRIVATE		__hidden
    #define CRASH_INVEST_IMPORT_FROM_DLL
#endif  // #ifdef _MSC_VER

#if defined(CRASH_INVEST_COMPILING_SHARED_LIB)
    #define CRASH_INVEST_EXPORT CRASH_INVEST_DLL_PUBLIC
#elif defined(CRASH_INVEST_USING_STATIC_LIB_OR_OBJECTS)
    #define CRASH_INVEST_EXPORT
#else
    #define CRASH_INVEST_EXPORT CRASH_INVEST_IMPORT_FROM_DLL
#endif

#if defined(CRASH_INVEST_HOOK_COMPILING_SHARED_LIB)
#define CRASH_INVEST_HOOK_EXPORT CRASH_INVEST_DLL_PUBLIC
#elif defined(CRASH_INVEST_HOOK_USING_STATIC_LIB_OR_OBJECTS)
#define CRASH_INVEST_HOOK_EXPORT
#else
#define CRASH_INVEST_HOOK_EXPORT CRASH_INVEST_IMPORT_FROM_DLL
#endif

#ifdef __cplusplus
#define CRASH_INVEST_BEGIN_C   extern "C" {
#define CRASH_INVEST_END_C     }
#define CRASH_INVEST_EXTERN_C  extern "C"
#else
#define CRASH_INVEST_BEGIN_C
#define CRASH_INVEST_END_C
#define CRASH_INVEST_EXTERN_C
#endif


#ifndef CRASH_INVEST_FALLTHROUGH
#ifdef CRASH_INVEST_CPP_17_DEFINED
#define CRASH_INVEST_FALLTHROUGH	[[fallthrough]] ;
#else
#define CRASH_INVEST_FALLTHROUGH	CRASH_INVEST_BEFORE_CPP_17_FALL_THR
//#define CRASH_INVEST_FALLTHROUGH
#endif
#endif

#ifndef CRASH_INVEST_NULL
#ifdef CRASH_INVEST_CPP_11_DEFINED
#define CRASH_INVEST_NULL	nullptr
#else
#define CRASH_INVEST_NULL	NULL
#endif
#endif

#ifndef CRASH_INVEST_REGISTER
#ifdef __cplusplus
#define CRASH_INVEST_REGISTER
#else
#define CRASH_INVEST_REGISTER	register
#endif
#endif

//#define CRASH_INVEST_IS_LITTLE_ENDIAN (((union { unsigned x; unsigned char c; }){1}).c)

#if defined(_DEBUG) || defined(CPPUTILS_DEBUG) || defined(CRASH_INVEST_DEBUG)
// we have debug compilation
#else
// we have release
#ifndef NDEBUG
// let's define NDEBUG (No DEBUG)
#define NDEBUG
#endif
#endif

#ifdef NDEBUG
//#define MAKE_VOID(_val)                 static_cast<void>(_val)
#define DO_DEBUG_EXP(_exp)
#define CRASH_INVEST_SAFE_CAST(_type,_val)	static_cast<_type>(_val)
#else
//#define MAKE_VOID(_var)                 do{}while(0)
//#define MAKE_VOID(_var)
#define DO_DEBUG_EXP(_exp)              _exp ;
#define CRASH_INVEST_SAFE_CAST(_type,_val)	dynamic_cast<_type>(_val)
#endif


#ifdef __cplusplus
#define CRASH_INVEST_STATIC_CAST(_type,_val)    static_cast<_type>(_val)
#else
#define CRASH_INVEST_STATIC_CAST(_type,_val)    ( (_type)(_val) )
#endif


#ifdef CRASH_INVEST_CPP_20_DEFINED
#define CRASH_INVEST_NODISCARD	[[nodiscard]]
#elif defined(CRASH_INVEST_CPP_11_DEFINED)
#define CRASH_INVEST_NODISCARD	noexcept
#else
#define CRASH_INVEST_NODISCARD	throw()
#endif

#ifdef CRASH_INVEST_CPP_11_DEFINED
#define CRASH_INVEST_NOEXCEPT	noexcept
#else
#define CRASH_INVEST_NOEXCEPT	throw()
#endif

#ifdef CRASH_INVEST_DO_NOT_USE_AT_ALL
#ifndef CRASH_INVEST_DO_NOT_USE_NEW_DEL
#define CRASH_INVEST_DO_NOT_USE_NEW_DEL
#endif
#ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE
#define CRASH_INVEST_DO_NOT_USE_MAL_FREE
#endif
#endif


#endif  // #ifndef INCLUDE_CRASH_INVEST_INTERNAL_HEADER_H
