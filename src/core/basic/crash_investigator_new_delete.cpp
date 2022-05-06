//
// file:		crash_investigator_alloc_dealloc.cpp
// path:		src/core/crash_investigator.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//


#include <crash_investigator/crash_investigator_internal_header.h>

#ifndef CRASH_INVEST_DO_NOT_USE_NEW_DEL

#include "crash_investigator_alloc_dealloc.hpp"
#include <new>

#ifdef _MSC_VER
#pragma comment(lib,"crash_investiator_new_malloc.lib")
#endif


void* operator new  ( std::size_t a_count ){
        return crash_investigator::TestOperatorAlloc(a_count,crash_investigator::MemoryType::New, true, 1);
}
void* operator new []  ( std::size_t a_count ){
        return crash_investigator::TestOperatorAlloc(a_count,crash_investigator::MemoryType::NewArr, true, 1);
}
void* operator new  ( std::size_t a_count, const std::nothrow_t& ) CPPUTILS_NODISCARD {
        return crash_investigator::TestOperatorAlloc(a_count,crash_investigator::MemoryType::New, false, 1);
}
void* operator new []  ( std::size_t a_count, const std::nothrow_t& ) CPPUTILS_NODISCARD{
        return crash_investigator::TestOperatorAlloc(a_count,crash_investigator::MemoryType::NewArr, false, 1);
}
void operator delete  ( void* a_ptr ) CPPUTILS_NOEXCEPT{
        crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::New,1);
}
void operator delete [] ( void* a_ptr ) CPPUTILS_NOEXCEPT{
        crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::NewArr,1);
}
void operator delete  ( void* a_ptr, std::size_t ) CPPUTILS_NOEXCEPT{
        crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::New,1);
}
void operator delete [] ( void* a_ptr, std::size_t ) CPPUTILS_NOEXCEPT{
        crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::NewArr,1);
}


#ifdef CPPUTILS_CPP_17_DEFINED

void* operator new  ( std::size_t a_count, std::align_val_t a_al ){
        return crash_investigator::TestOperatorNewAligned(a_count,crash_investigator::MemoryType::New,true,static_cast<std::size_t>(a_al),1);
}
void* operator new []  ( std::size_t a_count, std::align_val_t a_al ){
        return crash_investigator::TestOperatorNewAligned(a_count,crash_investigator::MemoryType::NewArr, true,static_cast<std::size_t>(a_al),1);
}
void* operator new  ( std::size_t a_count, std::align_val_t a_al, const std::nothrow_t& ) CPPUTILS_NODISCARD{
        return crash_investigator::TestOperatorNewAligned(a_count,crash_investigator::MemoryType::New, true,static_cast<std::size_t>(a_al),1);
}
void* operator new []  ( std::size_t a_count, std::align_val_t a_al, const std::nothrow_t& ) CPPUTILS_NODISCARD{
        return crash_investigator::TestOperatorNewAligned(a_count,crash_investigator::MemoryType::NewArr, true,static_cast<std::size_t>(a_al),1);
}

void operator delete  ( void* a_ptr, std::align_val_t  ) CPPUTILS_NOEXCEPT{
        crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::New,1);
}
void operator delete[]( void* a_ptr, std::align_val_t ) CPPUTILS_NOEXCEPT{
        crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::NewArr,1);
}

#endif  // #ifdef CPPUTILS_CPP_17_DEFINED

#endif  // #ifndef CPPUTILS_DO_NOT_USE_NEW_DEL
