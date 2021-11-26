//
// file:		crash_investigator_memory_items.cpp
// path:		src/core/crash_investigator_memory_items.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL


//#include "crash_investigator_alloc_dealloc.hpp"
#include <crash_investigator/core/rawallocfree.hpp>
#include <crash_investigator/core/backtrace.hpp>
#include <crash_investigator/callback.hpp>
//#include <cpputils/enums.hpp>
//#include <unordered_map>
#if defined(USE_CPPUTILS)
#include <cpputils/hashtbl.hpp>
#else
#include <cpputilsm/hashitemsbyptr.hpp>
#endif
#include <cpputils/inscopecleaner.hpp>
#include <mutex>
#include <stdio.h>
#ifdef CRASH_INVEST_CPP_17_DEFINED
#include <memory>
#endif
#ifdef CRASH_INVEST_DO_NOT_USE_MAL_FREE
#include <stdlib.h>
#include <malloc.h>
#endif

namespace crash_investigator {

#ifndef MY_NEW_PRINTF
//#define MY_NEW_PRINTF	printf
#define MY_NEW_PRINTF(...)
#endif

struct SMemoryItem;

static FailureAction DefaultFailureClbk(const FailureData& a_data);
//static void* InitFailureDataAndCallClbk(const SMemoryItem& a_item, MemoryType a_freeType, void* a_failureAddress,
//                                        size_t a_count, FailureType a_failureType, int a_goBackInTheStackCalc);
static void* InitFailureDataAndCallClbkRaw(const SMemoryItem& a_item, MemoryType a_freeType, void* a_failureAddress,
                                           size_t a_count, FailureType a_failureType, Backtrace* a_pBacktrace);


#if 0
typedef ::std::mutex  ClbkMutex;
#else
struct ClbkMutex{
    void lock(){}
    void unlock(){}
};
#endif


enum class MemoryStatus : uint32_t {
	Allocated,
	Deallocated,
    DuringReallocChanged=MemoryStatus::Deallocated,
};
//CPPUTILS_ENUM_FAST_RAW(251,MemoryStatus,uint32_t,Allocated,Deallocated);


struct SMemoryItem{
	MemoryType		type;
	MemoryStatus	status;
	void*			realAddress;
    Backtrace*      allocTrace;
    Backtrace*      deallocTrace;
};

static ClbkMutex    s_clbkMutex;
static SCallback    s_clbkData = {CRASH_INVEST_NULL,&DefaultFailureClbk};

static thread_local bool s_bIsAllocingOrDeallocing = false;
//static bool s_bIsAllocingOrDeallocing = false;
class IsAllocingHandler{
public:
	IsAllocingHandler(){s_bIsAllocingOrDeallocing=true;}
	~IsAllocingHandler(){s_bIsAllocingOrDeallocing=false;}
};

#ifdef USE_CPPUTILS
typedef cpputils::hashtbl::Base<void*,SMemoryItem>  TypeHashTbl;
#else
typedef cpputilsm::HashItemsByPtr<void*,SMemoryItem,&mallocn,&freen>  TypeHashTbl;
#endif
static TypeHashTbl	s_memoryItems;
static std::mutex	s_mutexForMap;


CRASH_INVEST_EXPORT SCallback ReplaceFailureClbk(const SCallback& a_newClbk)
{
    SCallback retClbk;
    ::std::lock_guard<ClbkMutex> aClbMutGuard(s_clbkMutex);
    retClbk.userData = s_clbkData.userData;
    retClbk.clbkFnc = s_clbkData.clbkFnc;
    s_clbkData.userData = a_newClbk.userData;
    s_clbkData.clbkFnc = a_newClbk.clbkFnc?a_newClbk.clbkFnc:(&DefaultFailureClbk);
    return retClbk;
}


CRASH_INVEST_EXPORT SCallback GetFailureClbk(void)
{
    ::std::lock_guard<ClbkMutex> aClbMutGuard(s_clbkMutex);
    return s_clbkData;
}


static void* AddNewAllocatedMemoryAndCleanOldEntryNoLock(MemoryType a_memoryType,void* a_pReturn,Backtrace* a_pBacktrace)
{
    const SMemoryItem aItem({a_memoryType,MemoryStatus::Allocated,a_pReturn,a_pBacktrace,nullptr});
    size_t unHash;
    TypeHashTbl::iterator memIter=s_memoryItems.FindEntry(a_pReturn,&unHash);
    if(memIter==TypeHashTbl::s_endIter){
        s_memoryItems.AddEntryWithKnownHash(a_pReturn,unHash,aItem);
    }
    else{
        FreeBacktraceData(aItem.deallocTrace);
        FreeBacktraceData(aItem.allocTrace);
        memIter->second = aItem;
    }
    return a_pReturn;
}


static void* AddNewAllocatedMemoryAndCleanOldEntry(MemoryType a_memoryType,void* a_pReturn,int a_goBackInTheStackCalc)
{
    Backtrace* pBacktrace = InitBacktraceDataForCurrentStack(++a_goBackInTheStackCalc);
    std::lock_guard<std::mutex> aGuard(s_mutexForMap);
    return AddNewAllocatedMemoryAndCleanOldEntryNoLock(a_memoryType,a_pReturn,pBacktrace);
}


CRASH_INVEST_DLL_PRIVATE void* TestOperatorAlloc  ( size_t a_count, MemoryType a_memoryType, bool a_bThrow, int a_goBackInTheStackCalc )
{
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::mallocn(a_count);}
	IsAllocingHandler aHandler;
	
	if(!a_count){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}
	
	void* pReturn = ::crash_investigator::mallocn(a_count);
	if(!pReturn){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CRASH_INVEST_NULL;}
	}

    return AddNewAllocatedMemoryAndCleanOldEntry(a_memoryType,pReturn,++a_goBackInTheStackCalc);
}


CRASH_INVEST_DLL_PRIVATE void* TestOperatorCalloc(size_t a_nmemb, size_t a_size, int a_goBackInTheStackCalc )
{
	if (s_bIsAllocingOrDeallocing) { return ::crash_investigator::callocn(a_nmemb,a_size); }
	IsAllocingHandler aHandler;

	if ((!a_nmemb)|| (!a_size)) { return CRASH_INVEST_NULL; }

	void* pReturn = ::crash_investigator::callocn(a_nmemb, a_size);
	if (!pReturn) {return CRASH_INVEST_NULL;}

    return AddNewAllocatedMemoryAndCleanOldEntry(MemoryType::Malloc,pReturn,++a_goBackInTheStackCalc);
}



CRASH_INVEST_DLL_PRIVATE void* TestOperatorReAlloc  ( void* a_ptr, size_t a_count, int a_goBackInTheStackCalc2 )
{
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::reallocn(a_ptr,a_count);}
    if(!a_ptr){return TestOperatorAlloc(a_count,MemoryType::Malloc,false,++a_goBackInTheStackCalc2);}
	IsAllocingHandler aHandler;
	
	if(!a_count){return CRASH_INVEST_NULL;}
	
	void* pReturn;
	TypeHashTbl::iterator memItemIter;

    Backtrace*const pAnalizeTrace = InitBacktraceDataForCurrentStack(++a_goBackInTheStackCalc2);
	

    {
        std::unique_lock<std::mutex> aGuard(s_mutexForMap);
		
        size_t unHash;
        memItemIter = s_memoryItems.FindEntry(a_ptr,&unHash);
		if(memItemIter==TypeHashTbl::s_endIter){
            SMemoryItem aItem = memItemIter->second;
            aGuard.unlock();
            return InitFailureDataAndCallClbkRaw(aItem,MemoryType::NotProvided,a_ptr,a_count,FailureType::BadReallocMemNotExist2,pAnalizeTrace);
		}

		if(memItemIter->second.type!=MemoryType::Malloc){
            SMemoryItem aItem = memItemIter->second;
            aGuard.unlock();
            return InitFailureDataAndCallClbkRaw(aItem,MemoryType::NotProvided,a_ptr,a_count,FailureType::BadReallocCreatedByWrongAlloc,pAnalizeTrace);
		}

		if(memItemIter->second.status!=MemoryStatus::Allocated){
            SMemoryItem aItem = memItemIter->second;
            aGuard.unlock();
            return InitFailureDataAndCallClbkRaw(aItem,MemoryType::NotProvided,a_ptr,a_count,FailureType::BadReallocDeletedMem2,pAnalizeTrace);
		}
		
		pReturn = ::crash_investigator::reallocn(memItemIter->second.realAddress,a_count) ;
		if(pReturn!=a_ptr){
            memItemIter->second.status = MemoryStatus::DuringReallocChanged;
            return AddNewAllocatedMemoryAndCleanOldEntryNoLock(MemoryType::Malloc,pReturn,pAnalizeTrace);
		}
	}

    FreeBacktraceData(pAnalizeTrace);
    return pReturn; // equal to a_ptr
}



CRASH_INVEST_DLL_PRIVATE void TestOperatorDelete(void* a_ptr, MemoryType a_typeExpected, int a_goBackInTheStackCalc ) CRASH_INVEST_NOEXCEPT
{
	if (s_bIsAllocingOrDeallocing) { ::crash_investigator::freen(a_ptr); return; } // not handling here
	IsAllocingHandler aHandler;

	TypeHashTbl::iterator memItemIter;
	void* pToDelete = CRASH_INVEST_NULL;
    Backtrace*const pAnalizeTrace = InitBacktraceDataForCurrentStack(++a_goBackInTheStackCalc);


    {
        std::unique_lock<std::mutex> aGuard(s_mutexForMap);

        size_t unHash;
        memItemIter = s_memoryItems.FindEntry(a_ptr,&unHash);

        // todo: early memory free should not be here!!! So changing temporary code
        //if (memItemIter == TypeHashTbl::s_endIter) { ::crash_investigator::freen(a_ptr); return; } // this is some early memory, leave this
		//if(memItemIter == TypeHashTbl::s_endIter){ return;}
        if(memItemIter==TypeHashTbl::s_endIter){
            SMemoryItem aItem = memItemIter->second;
            aGuard.unlock();
            InitFailureDataAndCallClbkRaw(aItem,MemoryType::NotProvided,a_ptr,0,FailureType::DeallocOfNonExistingMemory,pAnalizeTrace);
            return;
        }

		// we will  check 2 things a) if we used new, b) if this buffer is not deleted before
		if (memItemIter->second.status != MemoryStatus::Allocated) {            
            SMemoryItem aItem = memItemIter->second;
            aGuard.unlock();
            InitFailureDataAndCallClbkRaw(aItem,MemoryType::NotProvided,a_ptr,0,FailureType::DoubleFree,pAnalizeTrace);
            return;
		}

		if (memItemIter->second.type != a_typeExpected) {
            SMemoryItem aItem = memItemIter->second;
            aGuard.unlock();
            InitFailureDataAndCallClbkRaw(aItem,MemoryType::NotProvided,a_ptr,0,FailureType::FreeMissmatch,pAnalizeTrace);
            return;
		}

		pToDelete = memItemIter->second.realAddress;
		memItemIter->second.status = MemoryStatus::Deallocated;
        memItemIter->second.deallocTrace = pAnalizeTrace;
	}

	::crash_investigator::freen(pToDelete);
}


//#define CRASH_INVEST_CPP_17_DEFINED
#ifdef CRASH_INVEST_CPP_17_DEFINED

CRASH_INVEST_DLL_PRIVATE void* TestOperatorNewAligned(size_t a_count, MemoryType a_memoryType, bool a_bThrow, size_t a_align, int a_goBackInTheStackCalc)
{
	if (s_bIsAllocingOrDeallocing) { return ::crash_investigator::mallocn(a_count); }
	IsAllocingHandler aHandler;

	if (!a_count) {
		if (a_bThrow) { throw ::std::bad_alloc(); }
		else { return CRASH_INVEST_NULL; }
	}

	if (a_align < __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
		a_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
	}
	else {
		//printf("!!!!!!!!!!! alignment new is called count=%d, (all=%d), __STDCPP_DEFAULT_NEW_ALIGNMENT__=%d!\n",
		//	   (int)a_count, (int)a_align,__STDCPP_DEFAULT_NEW_ALIGNMENT__);
		fflush(stdout);
		uint64_t ullnAlign = static_cast<uint64_t>(a_align) - 1;
		ullnAlign |= (ullnAlign >> 1);
		ullnAlign |= (ullnAlign >> 2);
		ullnAlign |= (ullnAlign >> 4);
		ullnAlign |= (ullnAlign >> 8);
		ullnAlign |= (ullnAlign >> 16);
		ullnAlign |= (ullnAlign >> 32);
		++ullnAlign;
		if (static_cast<size_t>(ullnAlign) != a_align) {
			//fprintf(stderr,"6. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Wrong alignment size (%d). "
			//			   "Increasing to %d\n",static_cast<int>(a_align),static_cast<int>(ullnAlign));
			//fflush(stderr);
			//exit(1);
			a_align = static_cast<size_t>(ullnAlign);
		}
	}

	size_t actually_allocating = a_count + a_align + sizeof(void*);
    void* pReturn = ::crash_investigator::mallocn(actually_allocating);
    if (!pReturn) {
		if (a_bThrow) { throw ::std::bad_alloc(); }
		else { return CRASH_INVEST_NULL; }
	}

    void* pReturnRet = pReturn;
	MY_NEW_PRINTF("line:%d\n", __LINE__);

    if (std::align(a_align, a_count, pReturnRet, actually_allocating)) {
		MY_NEW_PRINTF("line:%d\n", __LINE__);
        AddNewAllocatedMemoryAndCleanOldEntry(a_memoryType,pReturn,++a_goBackInTheStackCalc);
        return pReturnRet;
	}

	if (a_bThrow) { throw ::std::bad_alloc(); }
	return CRASH_INVEST_NULL;
}

#endif  // #ifdef CRASH_INVEST_CPP_17_DEFINED


/*/////////////*/

static inline void PrintStack(const ::std::vector< StackItem>& a_stack)
{
    const size_t cunNumberOfFrames(a_stack.size());
    for(size_t i(0); i<cunNumberOfFrames;++i){
        printf("\t%p: %s\n",a_stack[i].address,a_stack[i].funcName.c_str());
    }
}


#if 0
enum class MemoryType : uint32_t{
    NotProvided,
    New,
    NewArr,
    Malloc,
};
#endif

static inline const char* AllocTypeFromMemoryType(MemoryType a_memoryType)
{
    switch (a_memoryType) {
    case MemoryType::New:
        return "new";
    case MemoryType::NewArr:
        return "new []";
    case MemoryType::Malloc:
        return "malloc/realloc/calloc";
    default:
        break;
    } // switch (a_memoryType) {

    return "NotProvided";
}


static FailureAction DefaultFailureClbk(const FailureData& a_data)
{
    switch(a_data.failureType){
    case FailureType::DoubleFree:
        printf("Address: %p invalid free() / delete / delete[]\n", a_data.failureAddress);
        PrintStack(a_data.analizeStack);
        printf("The previous deallocation was done in the following stack\n");
        PrintStack(a_data.stackFree);
        printf("Memory was created in the following stack\n");
        PrintStack(a_data.stackAlloc);
        break;
    case FailureType::BadReallocMemNotExist2:
        printf("Address: %p provided with realloc. The address is not a valid allocated address\n", a_data.failureAddress);
        PrintStack(a_data.analizeStack);
        break;
    case FailureType::BadReallocDeletedMem2:
        printf("Address: %p provided with realloc. The address was already deallocated\n", a_data.failureAddress);
        PrintStack(a_data.analizeStack);
        printf("The deallocation was done in the following stack\n");
        PrintStack(a_data.stackFree);
        printf("Memory was created in the following stack\n");
        PrintStack(a_data.stackAlloc);
        break;
    case FailureType::BadReallocCreatedByWrongAlloc:
        printf("Address: %p provided with realloc. The address was created by wrong routine (%s)\n",
               a_data.failureAddress,AllocTypeFromMemoryType(a_data.allocType));
        PrintStack(a_data.analizeStack);
        printf("Memory was created in the following stack\n");
        PrintStack(a_data.stackAlloc);
        break;
    case FailureType::FreeMissmatch:
        printf("Address: %p is being deallocated with non-proper routine. "
               "The memory allocated by %s and is being deallocated by %s\n",
               a_data.failureAddress, AllocTypeFromMemoryType(a_data.allocType),
               AllocTypeFromMemoryType(a_data.freeType));
        PrintStack(a_data.analizeStack);
        printf("Deallocation was done in the following stack\n");
        PrintStack(a_data.stackFree);
        printf("Memory was created in the following stack\n");
        PrintStack(a_data.stackAlloc);
        break;
    default:
        assert(false);
        break;
    } // switch(a_data.failureType){

    return FailureAction::ExitApp;
}




//static void* InitFailureDataAndCallClbk(const SMemoryItem& a_item, MemoryType a_freeType, void* a_failureAddress,
//                                        size_t a_count, FailureType a_failureType, int a_goBackInTheStackCalc)
//{
//    Backtrace*const pAnalizeTrace = InitBacktraceDataForCurrentStack(++a_goBackInTheStackCalc);
//    if(!pAnalizeTrace){
//        fprintf(stderr,"Unable to init backtrace!\n");
//        return CRASH_INVEST_NULL;
//    }
//
//    return InitFailureDataAndCallClbkRaw(a_item,a_freeType,a_failureAddress,a_count,a_failureType,pAnalizeTrace);
//}


#if 0
enum class FailureType : uint32_t{
    Unknown,
    DeallocOfNonExistingMemory,
    DoubleFree,
    BadReallocMemNotExist2,
    BadReallocDeletedMem2,
    BadReallocCreatedByWrongAlloc,
    FreeMissmatch,
};

struct StackItem{
    void*           address;
    ::std::string   funcName;
};


struct FailureData{
    FailureType                 failureType;
    MemoryType                  allocType;
    MemoryType                  freeType;
    uint32_t                    reserved01;
    mutable void*               clbkData;
    void*                       failureAddress;
    size_t                      badReallocSecondArg;
    ::std::vector< StackItem>   stackAlloc;
    ::std::vector< StackItem>   stackFree;
    ::std::vector< StackItem>   analizeStack;
};

#endif

static void* InitFailureDataAndCallClbkRaw(const SMemoryItem& a_item, MemoryType a_freeType, void* a_failureAddress,
                                           size_t a_count, FailureType a_failureType, Backtrace* a_pAnalizeTrace)
{
    FailureData aFailureData;
    FailureAction clbkRet;

    ::cpputils::InScopeCleaner aCleaner([a_pAnalizeTrace](void*){
        FreeBacktraceData(a_pAnalizeTrace);
    });

    aFailureData.failureType = a_failureType;

    switch(a_failureType){
    case FailureType::DeallocOfNonExistingMemory:
        aFailureData.allocType = MemoryType::NotProvided;
        aFailureData.freeType = a_freeType;
        aFailureData.reserved01 = 0;
        aFailureData.clbkData = s_clbkData.userData;
        aFailureData.failureAddress = a_failureAddress;
        aFailureData.badReallocSecondArg = 0;
        ConvertBacktraceToNames(a_pAnalizeTrace,&(aFailureData.analizeStack));

        clbkRet = (*s_clbkData.clbkFnc)(aFailureData);
        switch(clbkRet){
        case FailureAction::MakeAction:
            // ok if you want we will try our luck
            ::crash_investigator::freen(a_failureAddress);
        case FailureAction::DoNotMakeActionToPreventCrash:
            // preventing crash
            break;
        case FailureAction::ExitApp:
            // exiting app
            exit(1);
        default:
            fprintf(stderr, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            fflush(stderr);
            exit(1);
        }  // switch(clbkRet){
        return CRASH_INVEST_NULL;

    case FailureType::DoubleFree:
        aFailureData.allocType = a_item.type;
        aFailureData.freeType = a_freeType;
        aFailureData.reserved01 = 0;
        aFailureData.clbkData = s_clbkData.userData;
        aFailureData.failureAddress = a_failureAddress;
        aFailureData.badReallocSecondArg = 0;
        ConvertBacktraceToNames(a_item.allocTrace,&(aFailureData.stackAlloc));
        ConvertBacktraceToNames(a_item.deallocTrace,&(aFailureData.stackFree));
        ConvertBacktraceToNames(a_pAnalizeTrace,&(aFailureData.analizeStack));

        clbkRet = (*s_clbkData.clbkFnc)(aFailureData);
        switch(clbkRet){
        case FailureAction::MakeAction:
            // ok if you want we will try our luck
            ::crash_investigator::freen(a_failureAddress);
        case FailureAction::DoNotMakeActionToPreventCrash:
            // preventing crash
            break;
        case FailureAction::ExitApp:
            // exiting app
            exit(1);
        default:
            fprintf(stderr, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            fflush(stderr);
            exit(1);
        }  // switch(clbkRet){
        return CRASH_INVEST_NULL;

    case FailureType::BadReallocMemNotExist2:
        aFailureData.allocType = MemoryType::NotProvided;
        aFailureData.freeType = MemoryType::NotProvided;
        aFailureData.reserved01 = 0;
        aFailureData.clbkData = s_clbkData.userData;
        aFailureData.failureAddress = a_failureAddress;
        aFailureData.badReallocSecondArg = a_count;
        ConvertBacktraceToNames(a_pAnalizeTrace,&(aFailureData.analizeStack));

        clbkRet = (*s_clbkData.clbkFnc)(aFailureData);
        switch(clbkRet){
        case FailureAction::MakeAction:
            // ok if you want we will try our luck
            return ::crash_investigator::reallocn(a_failureAddress,a_count);
        case FailureAction::DoNotMakeActionToPreventCrash:
            // preventing crash
            break;
        case FailureAction::ExitApp:
            // exiting app
            exit(1);
        default:
            fprintf(stderr, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            fflush(stderr);
            exit(1);
        }  // switch(clbkRet){
        return CRASH_INVEST_NULL;

    case FailureType::BadReallocDeletedMem2:
        aFailureData.allocType = a_item.type;
        aFailureData.freeType = a_freeType;
        aFailureData.reserved01 = 0;
        aFailureData.clbkData = s_clbkData.userData;
        aFailureData.failureAddress = a_failureAddress;
        aFailureData.badReallocSecondArg = a_count;
        ConvertBacktraceToNames(a_item.allocTrace,&(aFailureData.stackAlloc));
        ConvertBacktraceToNames(a_item.deallocTrace,&(aFailureData.stackFree));
        ConvertBacktraceToNames(a_pAnalizeTrace,&(aFailureData.analizeStack));

        clbkRet = (*s_clbkData.clbkFnc)(aFailureData);
        switch(clbkRet){
        case FailureAction::MakeAction:
            // ok if you want we will try our luck
            return ::crash_investigator::reallocn(a_failureAddress,a_count);
        case FailureAction::DoNotMakeActionToPreventCrash:
            // preventing crash
            break;
        case FailureAction::ExitApp:
            // exiting app
            exit(1);
        default:
            fprintf(stderr, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            fflush(stderr);
            exit(1);
        }  // switch(clbkRet){
        return CRASH_INVEST_NULL;

    case FailureType::BadReallocCreatedByWrongAlloc:
        aFailureData.allocType = a_item.type;
        aFailureData.freeType = MemoryType::NotProvided;
        aFailureData.reserved01 = 0;
        aFailureData.clbkData = s_clbkData.userData;
        aFailureData.failureAddress = a_failureAddress;
        aFailureData.badReallocSecondArg = a_count;
        ConvertBacktraceToNames(a_item.allocTrace,&(aFailureData.stackAlloc));
        ConvertBacktraceToNames(a_pAnalizeTrace,&(aFailureData.analizeStack));

        clbkRet = (*s_clbkData.clbkFnc)(aFailureData);
        switch(clbkRet){
        case FailureAction::MakeAction:
            // ok if you want we will try our luck
            return ::crash_investigator::reallocn(a_failureAddress,a_count);
        case FailureAction::DoNotMakeActionToPreventCrash:
            // preventing crash
            break;
        case FailureAction::ExitApp:
            // exiting app
            exit(1);
        default:
            fprintf(stderr, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            fflush(stderr);
            exit(1);
        }  // switch(clbkRet){
        return CRASH_INVEST_NULL;

    case FailureType::FreeMissmatch:
        aFailureData.allocType = a_item.type;
        aFailureData.freeType = a_freeType;
        aFailureData.reserved01 = 0;
        aFailureData.clbkData = s_clbkData.userData;
        aFailureData.failureAddress = a_failureAddress;
        aFailureData.badReallocSecondArg = 0;
        ConvertBacktraceToNames(a_item.allocTrace,&(aFailureData.stackAlloc));
        ConvertBacktraceToNames(a_pAnalizeTrace,&(aFailureData.analizeStack));

        clbkRet = (*s_clbkData.clbkFnc)(aFailureData);
        switch(clbkRet){
        case FailureAction::MakeAction:
            // ok if you want we will try our luck
            ::crash_investigator::freen(a_failureAddress);
        case FailureAction::DoNotMakeActionToPreventCrash:
            // preventing crash
            break;
        case FailureAction::ExitApp:
            // exiting app
            exit(1);
        default:
            fprintf(stderr, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            fflush(stderr);
            exit(1);
        }  // switch(clbkRet){
        return CRASH_INVEST_NULL;

    default:
        assert(false);
        break;
    } // switch(a_failureType){

    return CRASH_INVEST_NULL;
}



} // namespace crash_investigator {

#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
