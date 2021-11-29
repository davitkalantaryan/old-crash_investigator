//
// file:			types.hpp
// path:			include/crash_investigator/types.hpp
// created on:		2021 Nov 25
// created by:		Davit Kalantaryan (davit.kalantaryan@gmail.com)
//

#pragma once

#include <crash_investigator/crash_investigator_internal_header.h>
//#include <cpputils/enums.hpp>
#include <stdint.h>


namespace crash_investigator {

enum class MemoryType : uint32_t{
    NotProvided,
    New,
    NewArr,
    Malloc,
};
//CPPUTILS_ENUM_FAST_RAW(142,MemoryType,uint32_t,NotHandled,New,NewArr);


} // namespace crash_investigator {
