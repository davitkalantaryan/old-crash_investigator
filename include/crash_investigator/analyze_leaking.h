//
// file:			crash_investigator_malloc_free_hook.h
// path:			include/crash_investigator/crash_investigator_malloc_free_hook.h
// created on:		2021 Nov 24
// created by:		Davit Kalantaryan (davit.kalantaryan@gmail.com)
//

#ifndef CRASH_INVEST_INCLUDE_CRASH_INVEST_ANALYZE_LEAKING_H
#define CRASH_INVEST_INCLUDE_CRASH_INVEST_ANALYZE_LEAKING_H

#include <crash_investigator/crash_investigator_internal_header.h>
#include <cinternal/lw_mutex_recursive.h>
#include <cinternal/hash/dllhash.h>
#include <time.h>
#include <stdbool.h>
#include <stddef.h>


CPPUTILS_BEGIN_C


struct SCrInvAnalyzeLeakingData {
    cinternal_lw_recursive_mutex_t  mutexForHashes;
    CinternalDLLHash_t              hashByAddress;
    CinternalDLLHash_t              hashByStack;
    int*                            pnIgnoreForThisThread;
    size_t                          countForThisStackMax;
    size_t                          unMaxValue;
    time_t                          initTimeSec;
    time_t                          deltaTimeToStartAnalyze;
    bool                            bInitializationTimeNotPassed;
    char                            reserved01[sizeof(void*) - sizeof(bool)];
};


CRASH_INVEST_EXPORT int  CrashInvestAnalyzeLeakingInitialize(struct SCrInvAnalyzeLeakingData* a_pTables, int*CPPUTILS_ARG_NN a_pnIgnoreForThisThread,
                                                            const char* a_cpcStartTimeEnv, const char* a_cpcMaxForStackEnv);
CRASH_INVEST_EXPORT void CrashInvestAnalyzeLeakingClean(struct SCrInvAnalyzeLeakingData* a_pTables);
CRASH_INVEST_EXPORT int  CrashInvestAnalyzeLeakingAddAllocedItem(int a_goBackInTheStackCalc, void* a_ptr, struct SCrInvAnalyzeLeakingData* a_pTables);
CRASH_INVEST_EXPORT void CrashInvestAnalyzeLeakingRemoveAllocedItem(void* a_ptr, struct SCrInvAnalyzeLeakingData* a_pTables);


CPPUTILS_END_C


#endif  // #ifndef CRASH_INVEST_INCLUDE_CRASH_INVEST_ANALYZE_LEAKING_H
