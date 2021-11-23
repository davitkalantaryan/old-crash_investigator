//
// file:		crash_investigator_memory_items.hpp
// path:		src/core/crash_investigator_memory_items.hpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#pragma once

#include <crash_investigator/crash_investigator_internal_header.h>
#include <cpputils/enums.hpp>
#include <stddef.h>
#include <stdint.h>

namespace crash_investigator {


enum class MemoryType : uint32_t{
	New,
	NewArr,
	Malloc,
};
//CPPUTILS_ENUM_FAST_RAW(142,MemoryType,uint32_t,NotHandled,New,NewArr);

CRASH_INVEST_DLL_PRIVATE void* TestOperatorNew  ( size_t a_count, MemoryType a_memoryType, bool a_bThrow );
#ifdef CRASH_INVEST_CPP_17_DEFINED
CRASH_INVEST_DLL_PRIVATE void* TestOperatorNewAligned  ( size_t a_count, MemoryType a_memoryType, bool a_bThrow, size_t a_align );
#endif
CRASH_INVEST_DLL_PRIVATE void  TestOperatorDelete( void* a_ptr, MemoryType a_typeExpected ) CRASH_INVEST_NOEXCEPT;
CRASH_INVEST_DLL_PRIVATE void* TestOperatorReAlloc  ( void* a_ptr, size_t a_count );


} // namespace crash_investigator {
