//
// file:		crash_investigator_memory_items.hpp
// path:		src/core/crash_investigator_memory_items.hpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#pragma once

#include <crash_investigator/crash_investigator_internal_header.h>
#include <crash_investigator/types.hpp>
#include <stddef.h>
#include <stdint.h>

namespace crash_investigator {


CRASH_INVEST_DLL_PRIVATE void* TestOperatorAlloc( size_t a_count, MemoryType a_memoryType, bool a_bThrow, int goBackInTheStackCalc );
CRASH_INVEST_DLL_PRIVATE void* TestOperatorCalloc(size_t a_nmemb, size_t a_size, int goBackInTheStackCalc);
CRASH_INVEST_DLL_PRIVATE void* TestOperatorReAlloc(void* a_ptr, size_t a_count, int goBackInTheStackCalc);
CRASH_INVEST_DLL_PRIVATE void  TestOperatorDelete(void* a_ptr, MemoryType a_typeExpected, int goBackInTheStackCalc) CRASH_INVEST_NOEXCEPT;
#ifdef CRASH_INVEST_CPP_17_DEFINED
CRASH_INVEST_DLL_PRIVATE void* TestOperatorNewAligned  ( size_t a_count, MemoryType a_memoryType, bool a_bThrow, size_t a_align, int goBackInTheStackCalc );
#endif


} // namespace crash_investigator {
