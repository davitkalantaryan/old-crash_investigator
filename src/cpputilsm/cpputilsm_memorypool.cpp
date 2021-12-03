//
// file:		cpputilsm_memorypool.cpp
// path:		src/cpputilsm/cpputilsm_memorypool.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Dec 03
//

#include <cpputilsm/memorypool.hpp>
#include <string.h>
#include <assert.h>

namespace cpputilsm{


void MemoryPoolBase::Init(void* a_pBuffer, size_t a_unBufferSize)
{
	m_pFirst = m_pLast = CRASH_INVEST_NULL;
	m_pBuffer = (static_cast<uint8_t*>(a_pBuffer));
	m_cunBufferSize = (a_unBufferSize);
	m_unOffset = (0);
}


void* MemoryPoolBase::Alloc(size_t a_count)
{
	const size_t cunOverallSize(a_count+sizeof(Item));
	if((cunOverallSize+m_unOffset)<m_cunBufferSize){
		uint8_t* pReturnRaw = m_pBuffer+m_unOffset;
		Item* pItem = reinterpret_cast<Item*>(pReturnRaw);
		if(m_pLast){
			m_pLast->next = pItem;
		}
		else{
			m_pFirst = pItem;
		}
		pItem->next = CRASH_INVEST_NULL;
		pItem->prev = m_pLast;
		m_pLast = pItem;
		pItem->count = a_count;
		pItem->isInUse = 1;
		m_unOffset += cunOverallSize;
		return static_cast<void*>(pReturnRaw+sizeof(Item));
	}
	return CRASH_INVEST_NULL;
}


void* MemoryPoolBase::Calloc(size_t a_nmemb, size_t a_size)
{
	const size_t cunCount(a_nmemb*a_size);
	void* pReturn = Alloc(cunCount);
	if(pReturn){
		memset(pReturn,0,cunCount);
	}
	return pReturn;
}


// return true, means correct memory
bool MemoryPoolBase::Realloc(void* a_ptr, size_t a_count, void** a_ppRet)
{
	if(a_ptr){
		const size_t ptrOffset(static_cast<uint8_t*>(a_ptr)-m_pBuffer);	
		if(ptrOffset<m_cunBufferSize){
			assert(ptrOffset>=sizeof(Item));
			if(a_count==0){
				*a_ppRet = CRASH_INVEST_NULL;
				DeallocCheckedMemory(ptrOffset);
				return true;
			}
			else{
				void* pReturnRaw = static_cast<void*>(static_cast<uint8_t*>(a_ptr)-sizeof(Item));
				if(a_count==static_cast<Item*>(pReturnRaw)->count){
					*a_ppRet = a_ptr;
					return true;
				}
				else{
					const size_t ofsetAfterAlloc = ptrOffset + static_cast<Item*>(pReturnRaw)->count;
					if(ofsetAfterAlloc==m_unOffset){
						// this is the last memory, let's simply extend if we have space
						const size_t cunNewOffset = static_cast<size_t>(  static_cast<ptrdiff_t>(ofsetAfterAlloc) + 
								static_cast<ptrdiff_t>(a_count) - static_cast<Item*>(pReturnRaw)->count  ) ;
						if(cunNewOffset<m_cunBufferSize){
							m_unOffset = cunNewOffset;
							static_cast<Item*>(pReturnRaw)->count = a_count;
							*a_ppRet = a_ptr;
							return true;
						}
						else{
							*a_ppRet = CRASH_INVEST_NULL;
							return true;
						}
					} // if(ofsetAfterAlloc==m_unOffset){
					else{
						assert(ofsetAfterAlloc<m_unOffset);
						DeallocCheckedMemory(ptrOffset);
						*a_ppRet = Alloc(a_count);
						return true;
					}
				} // else of if(a_count==static_cast<Item*>(pReturnRaw)->count){
			}  // else of if(a_count==0){
			
		}  // if(ptrOffset<m_cunBufferSize){
		
		*a_ppRet = CRASH_INVEST_NULL;
		return false;
	}
	
	*a_ppRet = Alloc(a_count);
	return true;
}


// return true, means correct memory
bool MemoryPoolBase::Dealloc(void* a_ptr)
{
	const size_t ptrOffset(static_cast<uint8_t*>(a_ptr)-m_pBuffer);
	if(ptrOffset<m_cunBufferSize){
		assert(ptrOffset>=sizeof(Item));
		DeallocCheckedMemory(ptrOffset);
		return true;
	}
	return false;
}


void MemoryPoolBase::DeallocCheckedMemory(size_t a_ptrOffset)
{
	size_t unRealPtrOffset( a_ptrOffset - sizeof(Item) );
	uint8_t* pReturnRaw = m_pBuffer + unRealPtrOffset;
	Item* pItem = reinterpret_cast<Item*>(pReturnRaw);
	const size_t ofsetAfterAlloc = a_ptrOffset + pItem->count;
	
	if(ofsetAfterAlloc==m_unOffset){
		// let's find all pending deallocations
		
		if(pItem==m_pFirst){
			m_unOffset = 0;
			m_pFirst = m_pLast = CRASH_INVEST_NULL;
			return;
		}
		
		Item* pItemPrev;
		pItem = pItem->prev;
		while(pItem){
			if(pItem->isInUse){
				pItem->next = CRASH_INVEST_NULL;
				m_pLast = pItem;
				break;
			}
			
			if(pItem==m_pFirst){
				m_unOffset = 0;
				m_pFirst = m_pLast = CRASH_INVEST_NULL;
				return;
			}
			
			pItemPrev = pItem->prev;
			unRealPtrOffset = reinterpret_cast<uint8_t*>(pItem)-m_pBuffer;
			//if(pItem->next){pItem->next->prev = pItem->prev;}
			//if(pItem->prev){pItem->prev->next = pItem->prev;}
			pItem = pItemPrev;
		}
		
		m_unOffset = unRealPtrOffset;
		return;
	}
	
	assert(ofsetAfterAlloc<m_unOffset);
	pItem->isInUse = 0;
}


}  // namespace cpputilsm{
