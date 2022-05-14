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


CRASH_INVEST_ALLOC_EXP void* TestOperatorAlloc( size_t a_count, MemoryType a_memoryType, bool a_bThrow, int goBackInTheStackCalc );
CRASH_INVEST_ALLOC_EXP void* TestOperatorCalloc(size_t a_nmemb, size_t a_size, int goBackInTheStackCalc);
CRASH_INVEST_ALLOC_EXP void* TestOperatorReAlloc(void* a_ptr, size_t a_count, int goBackInTheStackCalc);
CRASH_INVEST_ALLOC_EXP void  TestOperatorDelete(void* a_ptr, MemoryType a_typeExpected, int goBackInTheStackCalc) CPPUTILS_NOEXCEPT;
#ifdef CPPUTILS_CPP_17_DEFINED
CRASH_INVEST_ALLOC_EXP void* TestOperatorNewAligned  ( size_t a_count, MemoryType a_memoryType, bool a_bThrow, size_t a_align, int goBackInTheStackCalc );
#endif

#ifdef _MSC_VER
#define CorrectNameBase(_a,_b)      _a ## _b
#define CorrectName(_a)             CorrectNameBase(_a,WinMsC)
#else
#define CorrectName(_a)				_a
#endif


} // namespace crash_investigator {
