//
// file:		backtrace.hpp
// path:		src/include/crash_investigator/core/backtrace.hpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 25
//


#pragma once

#include <crash_investigator/crash_investigator_internal_header.h>
#include <crash_investigator/callback.hpp>
#include <stddef.h>
#include <stdbool.h>


namespace crash_investigator {

struct Backtrace;

CPPUTILS_DLL_PRIVATE Backtrace* InitBacktraceDataForCurrentStack(int goBackInTheStackCalc);
CPPUTILS_DLL_PRIVATE void ConvertBacktraceToNames(const Backtrace* data, ::std::vector< StackItem>*  pStack);
CPPUTILS_DLL_PRIVATE void FreeBacktraceData(Backtrace* data);
CPPUTILS_DLL_PRIVATE bool IsTheSameStack(const Backtrace* stack1, const Backtrace* stack2);
CPPUTILS_DLL_PRIVATE size_t HashOfTheStack(const Backtrace* stack);

}  // namespace crash_investigator {
