//
// file:		backtrace.hpp
// path:		src/include/crash_investigator/core/backtrace.hpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 25
//


#pragma once

#include <crash_investigator/crash_investigator_internal_header.h>
#include <crash_investigator/callback.hpp>


namespace crash_investigator {

struct Backtrace;

CPPUTILS_DLL_PRIVATE Backtrace* InitBacktraceDataForCurrentStack(int goBackInTheStackCalc);
CPPUTILS_DLL_PRIVATE void ConvertBacktraceToNames(const Backtrace* data, ::std::vector< StackItem>*  pStack);
CPPUTILS_DLL_PRIVATE void FreeBacktraceData(Backtrace* data);

}  // namespace crash_investigator {
