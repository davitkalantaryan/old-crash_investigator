
#ifndef CPPUTILSM_NEW_HASH_HPP
#define CPPUTILSM_NEW_HASH_HPP

#include <crash_investigator/crash_investigator_internal_header.h>
#include <new>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>

namespace cpputilsm{

#define CPPUTILSM_HASH_SIZE	4096


typedef void* (*TypeAlloc)  ( size_t );
typedef void  (*TypeFree)  ( void* );

template <typename TypeIntKey, typename TypeData, TypeAlloc allocFn= :: malloc, TypeFree freeFn= :: free>
class HashItemsByPtr
{
public:
	struct Item{
		TypeIntKey first;
		TypeData   second;
	private:
		size_t  unIndex;
		Item	*prev, *next;
		Item	*prevInTheList, *nextInTheList;
		friend class HashItemsByPtr;
	};
	class iterator{
	public:
		iterator(Item*);
		iterator();
		Item* operator->();
		bool operator==(const iterator&)const;
		bool operator!=(const iterator&)const;
		const iterator& operator++();
	private:
		Item* m_pItem;
		friend class HashItemsByPtr;
	};
	
	HashItemsByPtr();
	~HashItemsByPtr();
	
public:
    void     AddEntryWithKnownHash(const TypeIntKey& a_key, size_t a_hash, const TypeData& a_data);
    iterator FindEntry(const TypeIntKey& a_key, size_t* a_punSize);
	void     RemoveEntry(const iterator& iter);
	iterator begin();
	
	static void* operator new( ::std::size_t a_count );
	void operator delete  ( void* a_ptr ) CPPUTILS_NOEXCEPT ;
	
private:
    Item* FindEntryRaw(const TypeIntKey& a_key, size_t* a_punSize);
	
public:
	static iterator   s_endIter;
public:
	//Item**const		  m_ppItems;
	Item*		  m_ppItems[CPPUTILSM_HASH_SIZE];
	Item*			m_pFirstItem;
};

}  // namespace cpputilsm{


#ifndef CPPUTILSM_NEW_HASH_IMPL_HPP
#include "hashitemsbyptr.impl.hpp"
#endif


#endif  // #ifndef CPPUTILSM_NEW_HASH_HPP
