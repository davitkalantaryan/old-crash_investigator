//
// file:		crash_investigator_windows_missing_functions.h
// path:		src/core/crash_investigator_windows_missing_functions.h
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 24
//

#if defined(_WIN32) && !defined(CRASH_INVEST_DO_NOT_USE_AT_ALL)

#ifndef INCLUDE_CRASH_INVEST_WINDOWS_MISSING_FUNCTIONS_H
#define INCLUDE_CRASH_INVEST_WINDOWS_MISSING_FUNCTIONS_H

#include <crash_investigator/crash_investigator_internal_header.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <process.h>
#include <stdlib.h>
#include <assert.h>

#ifdef exit
#undef exit
#endif
#define exit(_code)		ExitProcess(CRASH_INVEST_STATIC_CAST(UINT,_code))

#ifdef assert
#undef assert
#endif
#define assert(_cond)	CRASH_INVEST_STATIC_CAST(void,_cond)


CRASH_INVEST_BEGIN_C


CRASH_INVEST_END_C

#endif  // #ifndef INCLUDE_CRASH_INVEST_WINDOWS_MISSING_FUNCTIONS_H

#endif  // #if defined(_WIN32) && !defined(CRASH_INVEST_DO_NOT_USE_AT_ALL)
