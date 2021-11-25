//
// file:		crash_investigator_windows_missing_functions.cpp
// path:		src/core/crash_investigator_windows_missing_functions.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 24
//

#if defined(_WIN32) && !defined(CRASH_INVEST_DO_NOT_USE_AT_ALL)

#include <crash_investigator/crash_investigator_internal_header.h>

#define CRASH_INVEST_INTERFACE_NOT_KNOWN

CRASH_INVEST_BEGIN_C


CRASH_INVEST_INTERFACE_NOT_KNOWN void _windows_crt_all_unknown_functionsWeak(void) {}
#pragma comment(linker, "/alternatename:_RTC_InitBase=_windows_crt_all_unknown_functionsWeak")
#pragma comment(linker, "/alternatename:_RTC_Shutdown=_windows_crt_all_unknown_functionsWeak")
#pragma comment(linker, "/alternatename:_DllMainCRTStartup=_windows_crt_all_unknown_functionsWeak")


void _windows_crt_all_known_functionsWeak(void) {}
#pragma comment(linker, "/alternatename:atexit=_windows_crt_all_known_functionsWeak")
#pragma comment(linker, "/alternatename:memset=_windows_crt_all_known_functionsWeak")


CRASH_INVEST_END_C


#endif    // #if defined(_WIN32) && !defined(CRASH_INVEST_DO_NOT_USE_AT_ALL)
