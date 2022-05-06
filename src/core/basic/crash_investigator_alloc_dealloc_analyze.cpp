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
#include <cpputilsm/hashitemsbyptr.hpp>
#include <cpputils/inscopecleaner.hpp>
#include <cpputils/tls_data.hpp>
#include <mutex>
#include <stdio.h>
#include <stdarg.h>
#ifdef CRASH_INVEST_CPP_17_DEFINED
#include <memory>
#endif
#include <string.h>
#include <signal.h>
#include <assert.h>


namespace crash_investigator {

struct SMemoryItem;

static FailureAction DefaultFailureClbk(const FailureData& a_data);
static int DefaultInfoReportStatic(void* clbkData,const char* a_format,...);
static int DefaultErrorReportStatic(void* clbkData,const char* a_format,...);
static void* InitFailureDataAndCallClbk(const SMemoryItem& a_item, MemoryType a_freeType, void* a_failureAddress,
                                        size_t a_count, FailureType a_failureType, Backtrace* a_pBacktrace);


enum class MemoryStatus : uint32_t {
	DoesNotExistAtAll,
	Allocated,
	Deallocated,
    DuringReallocChanged=static_cast<uint32_t>(MemoryStatus::Deallocated),
};
//CPPUTILS_ENUM_FAST_RAW(251,MemoryStatus,uint32_t,Allocated,Deallocated);

typedef Backtrace* BacktracePtr;

struct BackTrcHasher {
    size_t operator()(const BacktracePtr& a_intDt) const { return HashOfTheStack(a_intDt); }
};

struct BackTrcEql {
    bool operator()(const BacktracePtr& a_lhs, const BacktracePtr& a_rhs) const { return IsTheSameStack(a_lhs,a_rhs); }
};


struct SMemoryItem{
	MemoryType		type;
	MemoryStatus	status;
	void*			realAddress;
    Backtrace*      allocTrace;
    Backtrace*      deallocTrace;
};

static SCallback    s_clbkData = { CPPUTILS_NULL,&DefaultFailureClbk,&DefaultInfoReportStatic,&DefaultErrorReportStatic};

static thread_local bool s_bIsAllocingOrDeallocing = false;
class IsAllocingHandler{
public:
	IsAllocingHandler(){if(s_bIsAllocingOrDeallocing){m_bIsLocker=false;}else{s_bIsAllocingOrDeallocing=true;m_bIsLocker=true;}}
	~IsAllocingHandler(){if(m_bIsLocker){s_bIsAllocingOrDeallocing=false;}}
	
private:
	bool m_bIsLocker;
};


class CMemoryItem : public SMemoryItem{
public:
	CMemoryItem();
	~CMemoryItem();
	void Init(size_t a_count, FailureType a_failureType, void* a_prealAddress, Backtrace* a_pAnalizeTrace);
	FailureType failureType;
	size_t count;
};

typedef cpputilsm::HashItemsByPtr<void*,SMemoryItem, cpputilsm::IntHasher<void*>, cpputilsm::SmpEqual<void*>,&mallocn,&freen>  TypeHashTbl;
typedef cpputilsm::HashItemsByPtr<Backtrace*, int*, BackTrcHasher, BackTrcEql, &mallocn, &freen>  TypeStackTbl;


class CPPUTILS_DLL_PRIVATE CrashInvestAnalizerInit{
public:
    CrashInvestAnalizerInit();
	~CrashInvestAnalizerInit();
	
	TypeHashTbl								m_memoryItems;
	std::mutex								m_mutexForMap;
	cpputils::tls_ptr_fast<CMemoryItem>		m_handlerMemory;
    TypeStackTbl                            m_stackItems;
#ifdef _WIN32
	void (*m_funcInitial)(int);
#else
	struct sigaction m_saInitial;
#endif
}static s_crashInvestAnalizerInit;


// todo: remove below 2 lines and fix the code
static TypeHashTbl&	s_memoryItems = s_crashInvestAnalizerInit.m_memoryItems;
static std::mutex&	s_mutexForMap = s_crashInvestAnalizerInit.m_mutexForMap;


CRASH_INVEST_EXPORT SCallback ReplaceFailureClbk(const SCallback& a_newClbk)
{
    SCallback retClbk;
    //::std::lock_guard<ClbkMutex> aClbMutGuard(s_clbkMutex);
    retClbk.userData = s_clbkData.userData;
    retClbk.clbkFnc = s_clbkData.clbkFnc;
    retClbk.infoClbk = s_clbkData.infoClbk;
    retClbk.errorClbk = s_clbkData.errorClbk;
    s_clbkData.userData = a_newClbk.userData;
    s_clbkData.clbkFnc = a_newClbk.clbkFnc?a_newClbk.clbkFnc:(&DefaultFailureClbk);
    s_clbkData.infoClbk = a_newClbk.infoClbk?a_newClbk.infoClbk:(&DefaultInfoReportStatic);
    s_clbkData.errorClbk = a_newClbk.errorClbk?a_newClbk.errorClbk:(&DefaultErrorReportStatic);
    return retClbk;
}


CRASH_INVEST_EXPORT SCallback GetFailureClbk(void)
{
    return s_clbkData;
}


static inline void* AddNewAllocatedMemoryAndCleanOldEntryNoLock(MemoryType a_memoryType,void* a_pReturn,Backtrace* a_pBacktrace)
{
    const SMemoryItem aItem({a_memoryType,MemoryStatus::Allocated,a_pReturn,a_pBacktrace,nullptr});
    size_t unHash;
    TypeHashTbl::iterator memIter=s_memoryItems.FindEntry(a_pReturn,&unHash);
    if(memIter==TypeHashTbl::s_endIter){
        s_memoryItems.AddEntryWithKnownHash(a_pReturn,unHash,aItem);
    }
    else{
        //assert(false); // because of double free issue investigations, assert here is not true
        FreeBacktraceData(memIter->second.deallocTrace);
        FreeBacktraceData(memIter->second.allocTrace);
        memIter->second = aItem;
    }
    return a_pReturn;
}


static inline void* AddNewAllocatedMemoryAndCleanOldEntry(MemoryType a_memoryType,void* a_pReturn,int a_goBackInTheStackCalc)
{
    Backtrace* pBacktrace = InitBacktraceDataForCurrentStack(++a_goBackInTheStackCalc);
    std::lock_guard<std::mutex> aGuard(s_mutexForMap);
    return AddNewAllocatedMemoryAndCleanOldEntryNoLock(a_memoryType,a_pReturn,pBacktrace);
}

// todo: try to get information on correct behaviour when count is equal to 0
// #define CRASH_INVEST_ANALIZE_COUNT_0(_count, _bThrow)
#define CRASH_INVEST_ANALIZE_COUNT_0(_count, _bThrow)   \
    if((_count)==0){                                      \
        if(_bThrow){throw ::std::bad_alloc();}          \
        else{return CPPUTILS_NULL;}                 \
    }

CRASH_INVEST_ALLOC_EXP void TestOperatorDelete(void* a_ptr, MemoryType a_typeExpected, int a_goBackInTheStackCalc ) CPPUTILS_NOEXCEPT
{
	if(!a_ptr){return;}
	if (s_bIsAllocingOrDeallocing) { ::crash_investigator::freen(a_ptr); return; } // not handling here
	IsAllocingHandler aHandler;

	TypeHashTbl::iterator memItemIter;
	void* pToDelete = CPPUTILS_NULL;
    Backtrace*const pAnalizeTrace = InitBacktraceDataForCurrentStack(++a_goBackInTheStackCalc);


    {
        std::unique_lock<std::mutex> aGuard(s_mutexForMap);

        size_t unHash;
        memItemIter = s_memoryItems.FindEntry(a_ptr,&unHash);

        // todo: early memory free should not be here!!! So changing temporary code
        //if (memItemIter == TypeHashTbl::s_endIter) { ::crash_investigator::freen(a_ptr); return; } // this is some early memory, leave this
		//if(memItemIter == TypeHashTbl::s_endIter){ return;}
        if(memItemIter==TypeHashTbl::s_endIter){
			// in this case app will not exit if DefaultCallback is there
            aGuard.unlock();
			if(SystemSpecificLibInitialDealloc(a_ptr)){return;}
			
			// let's prepare temporar variable, that will be used if we have crash
			CMemoryItem* pMemoryItem = s_crashInvestAnalizerInit.m_handlerMemory.get();
			if(!pMemoryItem){
				pMemoryItem = new CMemoryItem;
				s_crashInvestAnalizerInit.m_handlerMemory = pMemoryItem;
			}
			pMemoryItem->Init(0,FailureType::DeallocOfNonExistingMemory,a_ptr,pAnalizeTrace);
			SystemSpecificGlibcDealloc(a_ptr);
            return;
        }

		// we will  check 2 things a) if we used new, b) if this buffer is not deleted before
		if (memItemIter->second.status != MemoryStatus::Allocated) {            
            SMemoryItem aItem = memItemIter->second;
            aGuard.unlock();
            InitFailureDataAndCallClbk(aItem,MemoryType::NotProvided,a_ptr,0,FailureType::DoubleFree,pAnalizeTrace);
            return;
		}

		if (memItemIter->second.type != a_typeExpected) {
            SMemoryItem aItem = memItemIter->second;
            aGuard.unlock();
            InitFailureDataAndCallClbk(aItem,MemoryType::NotProvided,a_ptr,0,FailureType::FreeMissmatch,pAnalizeTrace);
            return;
		}

		pToDelete = memItemIter->second.realAddress;
		memItemIter->second.status = MemoryStatus::Deallocated;
        memItemIter->second.deallocTrace = pAnalizeTrace;
	}

	::crash_investigator::freen(pToDelete);
}


CRASH_INVEST_ALLOC_EXP void* TestOperatorAlloc  ( size_t a_count, MemoryType a_memoryType, bool a_bThrow, int a_goBackInTheStackCalc )
{
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::mallocn(a_count);}
	IsAllocingHandler aHandler;
	
    CRASH_INVEST_ANALIZE_COUNT_0(a_count,a_bThrow)
	
	void* pReturn = ::crash_investigator::mallocn(a_count);
	if(!pReturn){
		if(a_bThrow){throw ::std::bad_alloc();}
		else{return CPPUTILS_NULL;}
	}

    return AddNewAllocatedMemoryAndCleanOldEntry(a_memoryType,pReturn,++a_goBackInTheStackCalc);
}


CRASH_INVEST_ALLOC_EXP void* TestOperatorCalloc(size_t a_nmemb, size_t a_size, int a_goBackInTheStackCalc )
{
	if (s_bIsAllocingOrDeallocing) { return ::crash_investigator::callocn(a_nmemb,a_size); }
	IsAllocingHandler aHandler;

    CRASH_INVEST_ANALIZE_COUNT_0(a_nmemb*a_size,false)

	void* pReturn = ::crash_investigator::callocn(a_nmemb, a_size);
	if (!pReturn) {return CPPUTILS_NULL;}

    return AddNewAllocatedMemoryAndCleanOldEntry(MemoryType::Malloc,pReturn,++a_goBackInTheStackCalc);
}



CRASH_INVEST_ALLOC_EXP void* TestOperatorReAlloc  ( void* a_ptr, size_t a_count, int a_goBackInTheStackCalc2 )
{
	if(s_bIsAllocingOrDeallocing){return ::crash_investigator::reallocn(a_ptr,a_count);}
    if(!a_ptr){return TestOperatorAlloc(a_count,MemoryType::Malloc,false,++a_goBackInTheStackCalc2);}
	
    //CRASH_INVEST_ANALIZE_COUNT_0(a_count,false)
	// in the case if 0 length provided, we delete initial memory
	if(!a_count){
		TestOperatorDelete(a_ptr,MemoryType::Malloc,++a_goBackInTheStackCalc2);
		return CPPUTILS_NULL;
	}

    IsAllocingHandler aHandler;
	
	void* pReturn;
	TypeHashTbl::iterator memItemIter;

    Backtrace*const pAnalizeTrace = InitBacktraceDataForCurrentStack(++a_goBackInTheStackCalc2);
	

    {
        std::unique_lock<std::mutex> aGuard(s_mutexForMap);
		
        size_t unHash;
        memItemIter = s_memoryItems.FindEntry(a_ptr,&unHash);
		if(memItemIter==TypeHashTbl::s_endIter){
            aGuard.unlock();
			void* pEarlyCall;
			if(SystemSpecificLibInitialRealloc(a_ptr,a_count,&pEarlyCall)){
				return pEarlyCall;
			}			
			
			// let's prepare temporar variable, that will be used if we have crash
			CMemoryItem* pMemoryItem = s_crashInvestAnalizerInit.m_handlerMemory.get();
			if(!pMemoryItem){
				pMemoryItem = new CMemoryItem;
				s_crashInvestAnalizerInit.m_handlerMemory = pMemoryItem;
			}
			pMemoryItem->Init(a_count,FailureType::BadReallocMemNotExist,a_ptr,pAnalizeTrace);
			return SystemSpecificGlibcRealloc(a_ptr,a_count);
		}

		if(memItemIter->second.type!=MemoryType::Malloc){
            SMemoryItem aItem = memItemIter->second;
            aGuard.unlock();
            return InitFailureDataAndCallClbk(aItem,MemoryType::NotProvided,a_ptr,a_count,FailureType::BadReallocCreatedByWrongAlloc,pAnalizeTrace);
		}

		if(memItemIter->second.status!=MemoryStatus::Allocated){
            SMemoryItem aItem = memItemIter->second;
            aGuard.unlock();
            return InitFailureDataAndCallClbk(aItem,MemoryType::NotProvided,a_ptr,a_count,FailureType::BadReallocDeletedMem,pAnalizeTrace);
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


#ifdef CPPUTILS_CPP_17_DEFINED

CRASH_INVEST_ALLOC_EXP void* TestOperatorNewAligned(size_t a_count, MemoryType a_memoryType, bool a_bThrow, size_t a_align, int a_goBackInTheStackCalc)
{
	if (s_bIsAllocingOrDeallocing) { return ::crash_investigator::mallocn(a_count); }
	IsAllocingHandler aHandler;

	if (!a_count) {
		if (a_bThrow) { throw ::std::bad_alloc(); }
		else { return CPPUTILS_NULL; }
	}

	if (a_align < __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
		a_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
	}
	else {
        //(*s_clbkData.infoClbk)(s_clbkData.userData,"!!!!!!!!!!! alignment new is called count=%d, (all=%d), __STDCPP_DEFAULT_NEW_ALIGNMENT__=%d!\n",
		//	   (int)a_count, (int)a_align,__STDCPP_DEFAULT_NEW_ALIGNMENT__);
		uint64_t ullnAlign = static_cast<uint64_t>(a_align) - 1;
		ullnAlign |= (ullnAlign >> 1);
		ullnAlign |= (ullnAlign >> 2);
		ullnAlign |= (ullnAlign >> 4);
		ullnAlign |= (ullnAlign >> 8);
		ullnAlign |= (ullnAlign >> 16);
		ullnAlign |= (ullnAlign >> 32);
		++ullnAlign;
		if (static_cast<size_t>(ullnAlign) != a_align) {
            //(*s_clbkData.errorClbk)(s_clbkData.userData,"6. !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Wrong alignment size (%d). "
			//			   "Increasing to %d\n",static_cast<int>(a_align),static_cast<int>(ullnAlign));
			//exit(1);
			a_align = static_cast<size_t>(ullnAlign);
		}
	}

	size_t actually_allocating = a_count + a_align + sizeof(void*);
    void* pReturn = ::crash_investigator::mallocn(actually_allocating);
    if (!pReturn) {
		if (a_bThrow) { throw ::std::bad_alloc(); }
		else { return CPPUTILS_NULL; }
	}

    void* pReturnRet = pReturn;
	MY_NEW_PRINTF("line:%d\n", __LINE__);

    if (std::align(a_align, a_count, pReturnRet, actually_allocating)) {
		MY_NEW_PRINTF("line:%d\n", __LINE__);
        AddNewAllocatedMemoryAndCleanOldEntry(a_memoryType,pReturn,++a_goBackInTheStackCalc);
        return pReturnRet;
	}

	if (a_bThrow) { throw ::std::bad_alloc(); }
	return CPPUTILS_NULL;
}

#endif  // #ifdef CPPUTILS_CPP_17_DEFINED





/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/


static inline void PrintStackSingleLine(const StackItem& a_stackItem)
{
	bool bLineOrSourceFile = false;
	(*s_clbkData.errorClbk)(s_clbkData.userData,"\t%p:  ",a_stackItem.address);
	if(a_stackItem.funcName.length()){
		(*s_clbkData.errorClbk)(s_clbkData.userData,"%s  ",a_stackItem.funcName.c_str());
	}
	if(a_stackItem.sourceFileName.length()){
		const char* cpcInitial = a_stackItem.sourceFileName.c_str();
        const char* flName = strrchr(cpcInitial, '\\');
        if (flName) { ++flName; }
        else {
            flName = strrchr(cpcInitial, '/');
            if (flName) { ++flName; }
            else {
                flName = cpcInitial;
            }
        }
		(*s_clbkData.errorClbk)(s_clbkData.userData,"(%s",flName);
		bLineOrSourceFile = true;
	}
	if(a_stackItem.line>0){
		if(bLineOrSourceFile){ (*s_clbkData.errorClbk)(s_clbkData.userData,":%d",a_stackItem.line); }
		else{ (*s_clbkData.errorClbk)(s_clbkData.userData,"(line:%d",a_stackItem.line);bLineOrSourceFile=true; }
	}
	if(bLineOrSourceFile){
		(*s_clbkData.errorClbk)(s_clbkData.userData,")  ",a_stackItem.line);
	}
	if(a_stackItem.dllName.length()){
		(*s_clbkData.errorClbk)(s_clbkData.userData,"(%s) ",a_stackItem.dllName.c_str());
	}
	(*s_clbkData.errorClbk)(s_clbkData.userData,"\n");
}


static inline void PrintStack(const ::std::vector< StackItem>& a_stack)
{
    const size_t cunNumberOfFrames(a_stack.size());
    for(size_t i(0); i<cunNumberOfFrames;++i){
		PrintStackSingleLine(a_stack[i]);
    }
}


/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

static void SignalSigsegvHandler(int)
{
	CMemoryItem* pMemoryItem = s_crashInvestAnalizerInit.m_handlerMemory.get();
	if(pMemoryItem){
		Backtrace*const pAnalizeTrace = pMemoryItem->deallocTrace;
		pMemoryItem->deallocTrace = CPPUTILS_NULL;
		InitFailureDataAndCallClbk(*pMemoryItem,MemoryType::NotProvided,pMemoryItem->realAddress,pMemoryItem->count,
								   pMemoryItem->failureType,pAnalizeTrace);
	}
	exit(3);
}


CrashInvestAnalizerInit::CrashInvestAnalizerInit()
{
	IsAllocingHandler aHandler;
	
#ifdef CRASH_INVEST_VERBOSE
	(*s_clbkData.infoClbk)(s_clbkData.userData, "+-+-+-+-+-+-+-+-+-+- Crash investigator lib version "
												CRASH_INVEST_VERSION_STR
												" +-+-+-+-+-+-+-+-+-+-\n");	
#endif
		
#ifdef _WIN32
	m_funcInitial = reinterpret_cast<void (*)(int)>(signal(SIGSEGV,&SignalSigsegvHandler));
#else
	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &SignalSigsegvHandler;
	if (sigaction(SIGSEGV, &sa, &m_saInitial) < 0 ){
		(*s_clbkData.errorClbk)(s_clbkData.userData, "!!!!!!!! Unable to change signal handler\n");
	}
#endif
	
}


CrashInvestAnalizerInit::~CrashInvestAnalizerInit()
{
	SMemoryItem* pMemItem;
	IsAllocingHandler aHandler;
	TypeHashTbl::iterator iter = s_memoryItems.begin();
	
	
#ifdef CRASH_INVEST_VERBOSE
	int nSizeOfUnallocated = 0;
	for(;iter!=TypeHashTbl::s_endIter;++iter){
		if(iter->second.status==MemoryStatus::Allocated){
			++nSizeOfUnallocated;
		}
	}
	
	::std::vector< StackItem> aStack;
	if(nSizeOfUnallocated){
		(*s_clbkData.infoClbk)(s_clbkData.userData, "\n\nProgram is about to finish. Number of unallocated memories %d\n",nSizeOfUnallocated);
	}
	
	for(;iter!=TypeHashTbl::s_endIter;++iter){
		pMemItem = &(iter->second);
		if(pMemItem->status==MemoryStatus::Allocated){
			ConvertBacktraceToNames(pMemItem->allocTrace,&aStack);
			PrintStack(aStack);
		}
		FreeBacktraceData(pMemItem->deallocTrace);
		FreeBacktraceData(pMemItem->allocTrace);
	}
#else
	for(;iter!=TypeHashTbl::s_endIter;++iter){
		pMemItem = &(iter->second);
		FreeBacktraceData(pMemItem->deallocTrace);
		FreeBacktraceData(pMemItem->allocTrace);
	}
#endif
	
#ifdef _WIN32
    signal(SIGSEGV, m_funcInitial);
#else
	sigaction(SIGSEGV, &m_saInitial, CRASH_INVEST_NULL);
#endif
}


/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

#if 0
struct SMemoryItem{
	MemoryType		type;
	MemoryStatus	status;
	void*			realAddress;
    Backtrace*      allocTrace;
    Backtrace*      deallocTrace;
};
#endif

CMemoryItem::CMemoryItem()
{
	this->type = MemoryType::NotProvided;
	this->status = MemoryStatus::DoesNotExistAtAll;
	this->allocTrace = CPPUTILS_NULL;
	this->deallocTrace = CPPUTILS_NULL;
}


CMemoryItem::~CMemoryItem()
{
	FreeBacktraceData(this->deallocTrace);
}


void CMemoryItem::Init(size_t a_count, FailureType a_failureType, void* a_prealAddress, Backtrace* a_pAnalizeTrace)
{
	FreeBacktraceData(this->deallocTrace);
	this->deallocTrace = a_pAnalizeTrace;
	this->realAddress = a_prealAddress;
	this->failureType = a_failureType;
	this->count = a_count;
}

/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/


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
    case FailureType::DeallocOfNonExistingMemory:
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Address: %p invalid deallocation. Address is not a valid allocated address\n", a_data.failureAddress);
        PrintStack(a_data.analizeStack);
        return FailureAction::DoNotMakeActionToPreventCrash;
    case FailureType::DoubleFree:
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Address: %p invalid free() / delete / delete[]\n", a_data.failureAddress);
        PrintStack(a_data.analizeStack);
        (*s_clbkData.errorClbk)(s_clbkData.userData,"The previous deallocation was done in the following stack\n");
        PrintStack(a_data.stackFree);
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Memory was created in the following stack\n");
        PrintStack(a_data.stackAlloc);
        break;
    case FailureType::BadReallocMemNotExist:
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Address: %p provided with realloc. The address is not a valid allocated address\n", a_data.failureAddress);
        PrintStack(a_data.analizeStack);
        break;
    case FailureType::BadReallocDeletedMem:
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Address: %p provided with realloc. The address was already deallocated\n", a_data.failureAddress);
        PrintStack(a_data.analizeStack);
        (*s_clbkData.errorClbk)(s_clbkData.userData,"The deallocation was done in the following stack\n");
        PrintStack(a_data.stackFree);
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Memory was created in the following stack\n");
        PrintStack(a_data.stackAlloc);
        break;
    case FailureType::BadReallocCreatedByWrongAlloc:
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Address: %p provided with realloc. The address was created by wrong routine (%s)\n",
                                a_data.failureAddress,AllocTypeFromMemoryType(a_data.allocType));
        PrintStack(a_data.analizeStack);
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Memory was created in the following stack\n");
        PrintStack(a_data.stackAlloc);
        break;
    case FailureType::FreeMissmatch:
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Address: %p is being deallocated with non-proper routine. "
                                                   "The memory allocated by %s and is being deallocated by %s\n",
                               a_data.failureAddress, AllocTypeFromMemoryType(a_data.allocType),
                               AllocTypeFromMemoryType(a_data.freeType));
        PrintStack(a_data.analizeStack);
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Deallocation was done in the following stack\n");
        PrintStack(a_data.stackFree);
        (*s_clbkData.errorClbk)(s_clbkData.userData,"Memory was created in the following stack\n");
        PrintStack(a_data.stackAlloc);
        break;
    default:
        assert(false);
        break;
    } // switch(a_data.failureType){

    return FailureAction::ExitApp;
}


static int DefaultInfoReportStatic(void*,const char* a_format,...)
{
    va_list ap;
    va_start(ap, a_format);
    int nRet = vfprintf(stdout,a_format,ap);
    va_end(ap);
    fflush(stdout);
    return nRet;
}

static int DefaultErrorReportStatic(void*,const char* a_format,...)
{
    va_list ap;
    va_start(ap, a_format);
    int nRet = vfprintf(stderr,a_format,ap);
    va_end(ap);
    fflush(stderr);
    return nRet;
}


static void* InitFailureDataAndCallClbk(const SMemoryItem& a_item, MemoryType a_freeType, void* a_failureAddress,
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
            (*s_clbkData.errorClbk)(s_clbkData.userData, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            exit(1);
        }  // switch(clbkRet){
        return CPPUTILS_NULL;

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
            (*s_clbkData.errorClbk)(s_clbkData.userData, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            exit(1);
        }  // switch(clbkRet){
        return CPPUTILS_NULL;

    case FailureType::BadReallocMemNotExist:
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
            (*s_clbkData.errorClbk)(s_clbkData.userData, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            exit(1);
        }  // switch(clbkRet){
        return CPPUTILS_NULL;

    case FailureType::BadReallocDeletedMem:
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
            (*s_clbkData.errorClbk)(s_clbkData.userData, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            exit(1);
        }  // switch(clbkRet){
        return CPPUTILS_NULL;

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
            (*s_clbkData.errorClbk)(s_clbkData.userData, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            exit(1);
        }  // switch(clbkRet){
        return CPPUTILS_NULL;

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
            (*s_clbkData.errorClbk)(s_clbkData.userData, "Bad FailureAction is provided (%d). Exiting app\n",static_cast<int>(clbkRet));
            exit(1);
        }  // switch(clbkRet){
        return CPPUTILS_NULL;

    default:
        assert(false);
        break;
    } // switch(a_failureType){

    return CPPUTILS_NULL;
}



} // namespace crash_investigator {

#endif  // #ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
