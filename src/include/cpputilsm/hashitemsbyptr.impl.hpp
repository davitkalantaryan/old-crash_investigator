
#ifndef CPPUTILSM_NEW_HASH_IMPL_HPP
#define CPPUTILSM_NEW_HASH_IMPL_HPP

#ifndef CPPUTILSM_NEW_HASH_HPP
#include "hashitemsbyptr.hpp"
#endif

#include <cassert>
#include <memory.h>
#include <stdio.h>


namespace cpputilsm{


// #define NEW_FREEE_USED
//#define MY_NEW_PRINTF	printf
#define MY_NEW_PRINTF(...)


//class HashItemsByPtr{
//	//
//};


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
typename HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::iterator   HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::s_endIter(CRASH_INVEST_NULL);

template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::HashItemsByPtr()
#ifdef NEW_FREEE_USED
	:
	  m_ppItems(static_cast<Item**>(allocFn(CPPUTILSM_HASH_SIZE*sizeof(Item*))))
#endif
{	
#ifdef NEW_FREEE_USED
	if(!m_ppItems){
		throw ::std::bad_alloc();
	}
#endif
	
	:: memset(m_ppItems,0,CPPUTILSM_HASH_SIZE*sizeof(Item*));
}


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::~HashItemsByPtr()
{
#ifdef NEW_FREEE_USED
	freeFn(m_ppItems);
#endif
}


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
void HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::AddOrReplaceEntry(const TypeIntKey& a_key, const TypeData& a_data)
{
	if(!m_ppItems){
		printf("file:%s, line:%d\n",__FILE__,__LINE__);
		return;
	}
	
	MY_NEW_PRINTF("file:%s, line:%d\n",__FILE__,__LINE__);
	Item* pItem = FindEntryRaw(a_key);
	MY_NEW_PRINTF("line:%d\n",__LINE__);
	if(pItem){
		pItem->second = a_data;
		return;
	}
	
	pItem = static_cast<Item*>(allocFn(sizeof(Item)));
	if(!pItem){
		throw ::std::bad_alloc();
	}
	
	pItem->unIndex = reinterpret_cast<size_t>(a_key)%CPPUTILSM_HASH_SIZE;
	pItem->prev = CRASH_INVEST_NULL;
	pItem->next = m_ppItems[pItem->unIndex];
	if(m_ppItems[pItem->unIndex]){
		m_ppItems[pItem->unIndex]->prev = pItem;
	}
	m_ppItems[pItem->unIndex] = pItem;
	new(&(pItem->first)) TypeIntKey(a_key);
	new(&(pItem->second)) TypeData(a_data);
}


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
typename HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::iterator HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::FindEntry(const TypeIntKey& a_key)
{
	Item* pItem = FindEntryRaw(a_key);
	return iterator(pItem);
}


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
void HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::RemoveEntry(const iterator& a_iter)
{
	assert(a_iter.m_pItem);
	a_iter.m_pItem->second.~TypeData();
	a_iter.m_pItem->first.~TypeIntKey();
	if(a_iter.m_pItem->next){a_iter.m_pItem->next->prev = a_iter.m_pItem->prev;}
	if(a_iter.m_pItem->prev){a_iter.m_pItem->prev->next = a_iter.m_pItem->next;}
	if(a_iter.m_pItem == m_ppItems[a_iter.m_pItem->unIndex]){m_ppItems[a_iter.m_pItem->unIndex]=a_iter.m_pItem->next;}
}


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
void* HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::operator new( ::std::size_t a_count )
{
	return allocFn(a_count);
}


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
void HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::operator delete  ( void* a_ptr ) CRASH_INVEST_NOEXCEPT
{
	freeFn(a_ptr);
}


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
typename HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::Item* HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::FindEntryRaw(const TypeIntKey& a_key)
{
	MY_NEW_PRINTF("file:%s, line:%d\n",__FILE__,__LINE__);
	size_t unIndex = reinterpret_cast<size_t>(a_key)%CPPUTILSM_HASH_SIZE;
	MY_NEW_PRINTF("file:%s, line:%d, m_ppItems=%p\n",__FILE__,__LINE__,m_ppItems);
	Item* pItem = m_ppItems[unIndex];
	MY_NEW_PRINTF("file:%s, line:%d\n",__FILE__,__LINE__);
	while(pItem){
		if(pItem->first == a_key){return pItem;}
		pItem = pItem->next;
	}
	return CRASH_INVEST_NULL;
}


/*//////////////////////////////////////////////////////////*/
template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::iterator::iterator(Item* a_pItem)
	:
	  m_pItem(a_pItem)
{
}


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::iterator::iterator()
	:
	  m_pItem(nullptr)
{
}


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
typename HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::Item* HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::iterator::operator->()
{
	return m_pItem;
}


template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn, TypeFree freeFn>
bool HashItemsByPtr<TypeIntKey,TypeData,allocFn,freeFn>::iterator::operator==(const iterator& a_aM)const
{
	return (m_pItem == a_aM.m_pItem);
}


}  // namespace cpputilsm{


#endif  // #ifndef CPPUTILSM_NEW_HASH_IMPL_HPP
