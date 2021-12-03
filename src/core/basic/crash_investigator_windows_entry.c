//
// file:		crash_investigator_alloc_dealloc.cpp
// path:		src/core/crash_investigator.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#if 0
#ifdef _WIN32
#ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL

#include <crash_investigator/crash_investigator_internal_header.h>
#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>


CRASH_INVEST_BEGIN_C

CRASH_INVEST_DLL_PUBLIC void Dummy(void) {}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved)  // reserved
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize once for each new process.
        // Return FALSE to fail DLL load.
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        break;

    case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
        break;

    case DLL_PROCESS_DETACH:
        // Perform any necessary cleanup.
        break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

CRASH_INVEST_END_C


#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
#endif  // #ifdef _WIN32
#endif
