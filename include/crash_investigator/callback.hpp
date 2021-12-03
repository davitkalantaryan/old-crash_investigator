//
// file:			callback.hpp
// path:			include/crash_investigator/callback.hpp
// created on:		2021 Nov 25
// created by:		Davit Kalantaryan (davit.kalantaryan@gmail.com)
//

#pragma once

#include <crash_investigator/crash_investigator_internal_header.h>
#include <crash_investigator/types.hpp>
//#include <cpputils/enums.hpp>
#include <vector>
#include <string>
#include <stdint.h>
#include <stddef.h>


namespace crash_investigator {

enum class FailureAction : uint32_t{
    Unknown,
    MakeAction,
    DoNotMakeActionToPreventCrash,
    ExitApp,
};

enum class FailureType : uint32_t{
    Unknown,
    DeallocOfNonExistingMemory,
    DoubleFree,
    BadReallocMemNotExist,
    BadReallocDeletedMem,
    BadReallocCreatedByWrongAlloc,
    FreeMissmatch,
};

struct StackItem{
    void*           address;
    ::std::string   dllName;
    ::std::string   funcName;
    ::std::string   sourceFileName;  // empty means unavailable
    int             line;      // <0 means unknown (no debug info available)
    int             reserved01;
};


struct FailureData{
    FailureType                 failureType;
    MemoryType                  allocType;
    MemoryType                  freeType;
    uint32_t                    reserved01;
    mutable void*               clbkData;
    void*                       failureAddress;
    size_t                      badReallocSecondArg;
    ::std::vector< StackItem>   stackAlloc;
    ::std::vector< StackItem>   stackFree;
    ::std::vector< StackItem>   analizeStack;
};

typedef FailureAction (*TypeFailureClbk)(const FailureData& data);
typedef int (*TypeReport)(void*,const char*,...);

struct SCallback{
    void*           userData;
    TypeFailureClbk clbkFnc;
    TypeReport      infoClbk;
    TypeReport      errorClbk;
};

#define CRASH_INVEST_CLBK(_userData,_clbkFnc)   ::crash_investigator::SCallback({(_userData),(clbkFnc),CRASH_INVEST_NULL,CRASH_INVEST_NULL})


CRASH_INVEST_EXPORT SCallback ReplaceFailureClbk(const SCallback& a_newClbk);
CRASH_INVEST_EXPORT SCallback GetFailureClbk(void);

} // namespace crash_investigator {

#ifdef _MSC_VER
CRASH_INVEST_BEGIN_C
CRASH_INVEST_EXPORT void crash_investiator_new_malloc_init(void);
CRASH_INVEST_END_C
#endif
