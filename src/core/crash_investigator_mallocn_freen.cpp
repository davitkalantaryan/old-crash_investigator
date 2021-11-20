//
// file:		crash_investigator_mallocn_freen.cpp
// path:		src/core/crash_investigator_mallocn_freen.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#include <crash_investigator/crash_investigator_internal_header.h>

#ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE

#include "crash_investigator_mallocn_freen.hpp"
#include <mutex>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>


namespace crash_investigator {

#define CRASH_INVEST_MEMORY_HANDLER_SIZE	1048576  // 1MB

static uint8_t  s_vInitialBuffer[CRASH_INVEST_MEMORY_HANDLER_SIZE];

class MemoryHandlerBase{
public:
	struct Item;
	MemoryHandlerBase(uint8_t* pBuffer);
	
	void* Alloc(size_t a_count) CRASH_INVEST_NOEXCEPT;
	void* Realloc(Item* a_pItem, void* a_ptr, size_t a_count) CRASH_INVEST_NOEXCEPT;
	void  Dealloc(Item* a_pItem) CRASH_INVEST_NOEXCEPT;
	bool  isOkToDelete()const;
	
private:
	void* AllocNoLock(size_t a_count) CRASH_INVEST_NOEXCEPT;
	void  DeallocNoLock(Item* a_pItem) CRASH_INVEST_NOEXCEPT;
	
public:
	struct Item{
		MemoryHandlerBase* pParent;
		size_t			   overallSize;
		Item			   *prev, *next;
	};
	
private:
	Item			*m_pFirst, *m_pLast;
	uint8_t*const	m_pBuffer;
	std::mutex		m_mut;
	size_t			m_remainingSize;
};


class MemoryHandler : public MemoryHandlerBase{
public:
	MemoryHandler(uint8_t* pBuffer);
	MemoryHandler	*prev, *next;
};

static constexpr size_t s_mmapSize = CRASH_INVEST_MEMORY_HANDLER_SIZE + sizeof(MemoryHandler);

static MemoryHandlerBase	s_bufferForHandling(s_vInitialBuffer);
static MemoryHandler*		s_firstHandler = nullptr;
static MemoryHandler*		s_lastHandler = nullptr;
static std::mutex			s_mutexForList;
static thread_local bool	s_bIsMmapOngoing = false;
//static  bool	s_bIsMmapOngoing = false;


class IsAllocingHandler02{
public:
	IsAllocingHandler02(){s_bIsMmapOngoing=true;}
	~IsAllocingHandler02(){s_bIsMmapOngoing=false;}
};


static void* MallocNoLock(size_t a_count) CRASH_INVEST_NOEXCEPT
{
	// if mmap is ongoing then no mmaped memory, only chance is static memory
	if(s_bIsMmapOngoing){
		return s_bufferForHandling.Alloc(a_count);
	}
	
	void* pReturn;	
	MemoryHandler* pHandler = s_lastHandler;
	
	while(pHandler){
		pReturn= pHandler->Alloc(a_count);
		if(pReturn){
			return pReturn;
		}
		pHandler = pHandler->prev;
	}
	
	{
		IsAllocingHandler02 aHandler;
		pReturn =  mmap(CRASH_INVEST_NULL, s_mmapSize, PROT_READ|PROT_WRITE, MAP_PRIVATE, -1, 0);
		if(!pReturn){return pReturn;}
	}
	
	pHandler = new (pReturn) MemoryHandler(static_cast<uint8_t*>(pReturn)+sizeof(MemoryHandler));
	
	pHandler->prev = s_lastHandler;
	if(s_lastHandler){
		s_lastHandler->next = pHandler;
	}
	else{
		s_firstHandler = pHandler;
	}
	s_lastHandler = pHandler;
	
	return pHandler->Alloc(a_count);
}


CRASH_INVEST_DLL_PRIVATE void* malloc  ( size_t a_count ) CRASH_INVEST_NOEXCEPT
{
	std::lock_guard<std::mutex> aGuard(s_mutexForList);
	return MallocNoLock(a_count);
}


CRASH_INVEST_DLL_PRIVATE void* realloc( void* a_ptr, size_t a_count ) CRASH_INVEST_NOEXCEPT
{
	std::lock_guard<std::mutex> aGuard(s_mutexForList);
	if(a_ptr){
		MemoryHandlerBase::Item*const pItem = reinterpret_cast<MemoryHandlerBase::Item*>(static_cast<uint8_t*>(a_ptr)-sizeof(MemoryHandlerBase::Item));
		void* pReturn = pItem->pParent->Realloc(pItem,a_ptr,a_count);
		if(pReturn){return pReturn;}
		pReturn = MallocNoLock(a_count);
		if(pReturn){
			const size_t unOldCount(pItem->overallSize - sizeof(MemoryHandlerBase::Item));
			if(a_count>unOldCount){a_count=unOldCount;}
			memcpy(pReturn,a_ptr,a_count);
			pItem->pParent->Dealloc(pItem);
		}
		return pReturn;
	}
	return MallocNoLock(a_count);
}


CRASH_INVEST_DLL_PRIVATE void free( void* a_ptr ) CRASH_INVEST_NOEXCEPT
{
	if(a_ptr){
		std::lock_guard<std::mutex> aGuard(s_mutexForList);
		MemoryHandlerBase::Item*const pItem = reinterpret_cast<MemoryHandlerBase::Item*>(static_cast<uint8_t*>(a_ptr)-sizeof(MemoryHandlerBase::Item));
		pItem->pParent->Dealloc(pItem);
		if(pItem->pParent->isOkToDelete()&&(pItem->pParent!=(&s_bufferForHandling))){
			MemoryHandler* pHandler = static_cast<MemoryHandler*>(pItem->pParent);
			if(pHandler->prev){pHandler->prev->next=pHandler->next;}
			if(pHandler->next){pHandler->next->prev=pHandler->prev;}
			if(s_firstHandler == pHandler){s_firstHandler=pHandler->next;}
			if(s_lastHandler == pHandler){s_lastHandler=pHandler->prev;}
			munmap(pItem->pParent,s_mmapSize);
		}
	}
}


//
MemoryHandlerBase::MemoryHandlerBase(uint8_t* a_pBuffer)
	:
	  m_pBuffer(a_pBuffer)
{
	m_remainingSize = CRASH_INVEST_MEMORY_HANDLER_SIZE;
	m_pLast = m_pFirst = CRASH_INVEST_NULL;
}


void* MemoryHandlerBase::Alloc(size_t a_count) CRASH_INVEST_NOEXCEPT
{
	std::lock_guard<std::mutex> aGuard(m_mut);
	return AllocNoLock(a_count);
}


void* MemoryHandlerBase::AllocNoLock(size_t a_count) CRASH_INVEST_NOEXCEPT
{
	const size_t unOverallSize(a_count + sizeof(Item));
	if(unOverallSize>m_remainingSize){return CRASH_INVEST_NULL;}
	const size_t unOffset(CRASH_INVEST_MEMORY_HANDLER_SIZE-m_remainingSize);
	m_remainingSize -= unOverallSize;
	Item* pItem = reinterpret_cast<Item*>(m_pBuffer+unOffset);
	pItem->pParent = this;
	pItem->overallSize = unOverallSize;
	pItem->prev = m_pLast;
	if(m_pLast){
		m_pLast->prev = pItem;
	}
	else{
		m_pFirst = pItem;
	}
	m_pLast = pItem;
	return reinterpret_cast<uint8_t*>(pItem)+sizeof(Item);
}


void* MemoryHandlerBase::Realloc(Item* a_pItem, void* a_ptr, size_t a_count) CRASH_INVEST_NOEXCEPT
{
	const size_t unOverallSize(a_count + sizeof(Item));
	std::lock_guard<std::mutex> aGuard(m_mut);
	
	if((a_pItem->next==CRASH_INVEST_NULL)&&(unOverallSize<=m_remainingSize)){
		m_remainingSize += a_pItem->overallSize;
		m_remainingSize -= unOverallSize;
		a_pItem->overallSize = unOverallSize;
		return a_ptr;
	}
	
	if(unOverallSize<=a_pItem->overallSize){
		// no need to decrease item size
		return a_ptr;
	}
		
	// we have to allocate new memory
	void* pReturn = AllocNoLock(a_count);
	if(pReturn){
		const size_t unOldCount = a_pItem->overallSize - sizeof(Item);
		if(a_count>unOldCount){a_count=unOldCount;}
		memcpy(pReturn,a_ptr,a_count);
		DeallocNoLock(a_pItem);
	}
	return pReturn;
}


void MemoryHandlerBase::Dealloc(Item* a_pItem) CRASH_INVEST_NOEXCEPT
{
	std::lock_guard<std::mutex> aGuard(m_mut);
	DeallocNoLock(a_pItem);
}


void MemoryHandlerBase::DeallocNoLock(Item* a_pItem) CRASH_INVEST_NOEXCEPT
{
	if(a_pItem->next){
		if(a_pItem->prev){
			a_pItem->prev->overallSize += a_pItem->overallSize;
		}
		else{
			m_pFirst = a_pItem->next;
		}
		a_pItem->next->prev = a_pItem->prev;
	}
	else{
		m_pLast = a_pItem->prev;
		if(m_pLast){
			m_pLast->overallSize += a_pItem->overallSize;
			m_pLast->next = CRASH_INVEST_NULL;
		}
		else{
			m_pFirst = CRASH_INVEST_NULL;
			m_remainingSize = CRASH_INVEST_MEMORY_HANDLER_SIZE;
		}
	}	
}


bool MemoryHandlerBase::isOkToDelete()const
{
	return m_pFirst==CRASH_INVEST_NULL;
}


MemoryHandler::MemoryHandler(uint8_t* a_pBuffer)
	:
	  MemoryHandlerBase(a_pBuffer)
{
	this->next = this->prev = CRASH_INVEST_NULL;
}


} // namespace crash_investigator {


#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_MAL_FREE
