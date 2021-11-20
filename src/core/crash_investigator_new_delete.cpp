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


void* operator new  ( std::size_t a_count ){
	return crash_investigator::TestOperatorNew(a_count,crash_investigator::MemoryType::New, true);
}
void* operator new []  ( std::size_t a_count ){
	return crash_investigator::TestOperatorNew(a_count,crash_investigator::MemoryType::NewArr, true);
}
void* operator new  ( std::size_t a_count, const std::nothrow_t& ) CRASH_INVEST_NODISCARD {
	return crash_investigator::TestOperatorNew(a_count,crash_investigator::MemoryType::New, false);
}
void* operator new []  ( std::size_t a_count, const std::nothrow_t& ) CRASH_INVEST_NODISCARD{
	return crash_investigator::TestOperatorNew(a_count,crash_investigator::MemoryType::NewArr, false);
}
void operator delete  ( void* a_ptr ) noexcept{
	crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::New);
}
void operator delete [] ( void* a_ptr ) noexcept{
	crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::NewArr);
}
void operator delete  ( void* a_ptr, std::size_t ) CRASH_INVEST_NOEXCEPT{
	crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::New);
}
void operator delete [] ( void* a_ptr, std::size_t ) CRASH_INVEST_NOEXCEPT{
	crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::NewArr);
}


#ifdef CRASH_INVEST_CPP_17_DEFINED

void* operator new  ( std::size_t a_count, std::align_val_t ){
	return crash_investigator::TestOperatorNew(a_count,crash_investigator::MemoryType::New, true);
}
void* operator new []  ( std::size_t a_count, std::align_val_t ){
	return crash_investigator::TestOperatorNew(a_count,crash_investigator::MemoryType::NewArr, true);
}
void* operator new  ( std::size_t a_count, std::align_val_t, const std::nothrow_t& ) CRASH_INVEST_NODISCARD{
	return crash_investigator::TestOperatorNew(a_count,crash_investigator::MemoryType::New, true);
}
void* operator new []  ( std::size_t a_count, std::align_val_t, const std::nothrow_t& ) CRASH_INVEST_NODISCARD{
	return crash_investigator::TestOperatorNew(a_count,crash_investigator::MemoryType::NewArr, true);
}

void operator delete  ( void* a_ptr, std::align_val_t  ) CRASH_INVEST_NOEXCEPT{
	crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::New);
}
void operator delete[]( void* a_ptr, std::align_val_t ) CRASH_INVEST_NOEXCEPT{
	crash_investigator::TestOperatorDelete(a_ptr,crash_investigator::MemoryType::NewArr);
}

#endif  // #ifdef CRASH_INVEST_CPP_17_DEFINED

#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_NEW_DEL
