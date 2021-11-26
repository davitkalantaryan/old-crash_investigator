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
    BadReallocMemNotExist2,
    BadReallocDeletedMem2,
    BadReallocCreatedByWrongAlloc,
    FreeMissmatch,
};

struct StackItem{
    void*           address;
    ::std::string   funcName;
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

// return false, means
typedef FailureAction (*TypeFailureClbk)(const FailureData& data);

struct SCallback{
    void*           userData;
    TypeFailureClbk clbkFnc;
};


CRASH_INVEST_EXPORT SCallback ReplaceFailureClbk(const SCallback& a_newClbk);
CRASH_INVEST_EXPORT SCallback GetFailureClbk(void);

} // namespace crash_investigator {
