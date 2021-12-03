
#ifndef CPPUTILSM_MEMORYPOOL_HPP
#define CPPUTILSM_MEMORYPOOL_HPP

#include <crash_investigator/crash_investigator_internal_header.h>
#include <new>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>

namespace cpputilsm{


class MemoryPoolBase
{
public:	
	void* Alloc(size_t a_count);
	void* Calloc(size_t a_nmemb, size_t a_size); 
	bool  Realloc(void* a_ptr, size_t a_count, void** a_pRet); // return true, means correct memory
	bool  Dealloc(void* a_ptr);   // return true, means correct memory
	
protected:
	void  Init(void* a_pBuffer, size_t a_unBufferSize);
	
private:
	void  DeallocCheckedMemory(size_t ptrOffset);
	
private:
	struct Item;
	
private:
	uint8_t*		m_pBuffer;
	size_t			m_cunBufferSize;
	size_t			m_unOffset;
	Item			*m_pFirst, *m_pLast;
	
private:
	struct Item{
		size_t count;
		size_t isInUse;
		Item *prev, *next;
	};
};

template <size_t BufferSize>
class MemoryPool : public MemoryPoolBase
{
public:
	MemoryPool();
	
private:
	uint8_t		m_vBuffer[BufferSize];
};


}  // namespace cpputilsm{


#ifndef CPPUTILSM_MEMORYPOOL_IMPL_HPP
#include "memorypool.impl.hpp"
#endif


#endif  // #ifndef CPPUTILSM_MEMORYPOOL_HPP
