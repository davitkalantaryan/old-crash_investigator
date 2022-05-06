
#ifndef CPPUTILSM_NEW_HASH_IMPL_HPP
#define CPPUTILSM_NEW_HASH_IMPL_HPP

#ifndef CPPUTILSM_NEW_HASH_HPP
#include "hashitemsbyptr.hpp"
#endif

#include <cassert>
#include <memory.h>
#include <stdio.h>


namespace cpputilsm{


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
typename HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::iterator   HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::s_endIter(CPPUTILS_NULL);

template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::HashItemsByPtr()
	:
#ifdef NEW_FREEE_USED
	  m_ppItems(static_cast<Item**>(allocFn(CPPUTILSM_HASH_SIZE*sizeof(Item*)))),
#endif
	  m_pFirstItem(CPPUTILS_NULL)
{	
#ifdef NEW_FREEE_USED
	if(!m_ppItems){
		throw ::std::bad_alloc();
	}
#endif
	
	:: memset(m_ppItems,0,CPPUTILSM_HASH_SIZE*sizeof(Item*));
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::~HashItemsByPtr()
{
	Item *pItemNext, *pItem = m_pFirstItem;
	
	while(pItem){
		pItemNext = pItem->nextInTheList;
		pItem->second.~TypeData();
		pItem->first.~TypeIntKey();
		freeFn(pItem);
		pItem = pItemNext;
	}
	
#ifdef NEW_FREEE_USED
	freeFn(m_ppItems);
#endif
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
void HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::
AddEntryWithKnownHash(const TypeIntKey& a_key, size_t a_hash, const TypeData& a_data)
{	
#ifdef NEW_FREEE_USED
	if(!m_ppItems){
		printf("file:%s, line:%d\n",__FILE__,__LINE__);
		return;
	}
#endif
		
    Item* pItem = static_cast<Item*>(allocFn(sizeof(Item)));
	if(!pItem){
		throw ::std::bad_alloc();
	}
	
    pItem->unIndex = a_hash;
	//
	pItem->prev = CPPUTILS_NULL;
	pItem->next = m_ppItems[pItem->unIndex];
	if(m_ppItems[pItem->unIndex]){
		m_ppItems[pItem->unIndex]->prev = pItem;
	}
	m_ppItems[pItem->unIndex] = pItem;
	//
	pItem->prevInTheList = CPPUTILS_NULL;
	pItem->nextInTheList = m_pFirstItem;
	if(m_pFirstItem){
		m_pFirstItem->prev = pItem;
	}
	m_pFirstItem = pItem;
	//
	new(&(pItem->first)) TypeIntKey(a_key);
	new(&(pItem->second)) TypeData(a_data);
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
typename HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::iterator HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::
FindEntry(const TypeIntKey& a_key,size_t* a_punSize)
{
    Item* pItem = FindEntryRaw(a_key,a_punSize);
	return iterator(pItem);
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
typename HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::iterator HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::
begin()
{
	return iterator(m_pFirstItem);
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
void HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::
RemoveEntry(const iterator& a_iter)
{
	if(a_iter.m_pItem){
		//
		a_iter.m_pItem->second.~TypeData();
		a_iter.m_pItem->first.~TypeIntKey();
		//
		if(a_iter.m_pItem->next){a_iter.m_pItem->next->prev = a_iter.m_pItem->prev;}
		if(a_iter.m_pItem->prev){a_iter.m_pItem->prev->next = a_iter.m_pItem->next;}
		if(a_iter.m_pItem == m_ppItems[a_iter.m_pItem->unIndex]){m_ppItems[a_iter.m_pItem->unIndex]=a_iter.m_pItem->next;}
		//
		if(a_iter.m_pItem->nextInTheList){a_iter.m_pItem->nextInTheList->prevInTheList = a_iter.m_pItem->prevInTheList;}
		if(a_iter.m_pItem->prevInTheList){a_iter.m_pItem->prevInTheList->nextInTheList = a_iter.m_pItem->nextInTheList;}
		if(a_iter.m_pItem == m_pFirstItem){m_pFirstItem=a_iter.m_pItem->nextInTheList;}
		//
		freeFn(a_iter.m_pItem);
		a_iter.m_pItem = CPPUTILS_NULL;
	}
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
typename HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::Item* HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::
FindEntryRaw(const TypeIntKey& a_key, size_t* a_punSize)
{
	const HashT aHash;
    *a_punSize = (aHash(a_key))%CPPUTILSM_HASH_SIZE;
    Item* pItem = m_ppItems[*a_punSize];
	while(pItem){
		if(pItem->first == a_key){return pItem;}
		pItem = pItem->next;
	}
	return CPPUTILS_NULL;
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
void* HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::operator new( ::std::size_t a_count )
{
	return allocFn(a_count);
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
void HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::operator delete  ( void* a_ptr ) CPPUTILS_NOEXCEPT
{
	freeFn(a_ptr);
}


/*//////////////////////////////////////////////////////////*/
template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::iterator::iterator(Item* a_pItem)
	:
	  m_pItem(a_pItem)
{
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::iterator::iterator()
	:
	  m_pItem(nullptr)
{
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
const typename HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::iterator& HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::
iterator::operator++()
{
	m_pItem = m_pItem->nextInTheList;
	return *this;
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
typename HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::Item* HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::
iterator::operator->()
{
	return m_pItem;
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
bool HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::
iterator::operator==(const iterator& a_aM)const
{
	return (m_pItem == a_aM.m_pItem);
}


template <typename TypeIntKey, typename TypeData, typename HashT, TypeAlloc allocFn, TypeFree freeFn>
bool HashItemsByPtr<TypeIntKey,TypeData, HashT,allocFn,freeFn>::
iterator::operator!=(const iterator& a_aM)const
{
	return (m_pItem != a_aM.m_pItem);
}


}  // namespace cpputilsm{


#endif  // #ifndef CPPUTILSM_NEW_HASH_IMPL_HPP
