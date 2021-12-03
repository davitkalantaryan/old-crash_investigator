
#include <crash_investigator/crash_investigator_internal_header.h>
#include "crash_investigator_malloc_free_hook.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdio.h>


static bool CrashInvestHookClbk(const CrashInvestClbkArg* data)
{
    printf("CrashInvestHookClbk\n");
    return true;
}

CRASH_INVEST_BEGIN_C

CRASH_INVEST_DLL_PUBLIC void CrashInvestDummy2(void) {}

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
        CrashInvestSetNewHookClbk(CRASH_INVEST_NULL,CrashInvestHookClbk);
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
