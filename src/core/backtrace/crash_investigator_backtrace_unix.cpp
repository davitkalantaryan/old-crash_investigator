//
// file:		crash_investigator_backtrace_unix.cpp
// path:		src/core/backtrace/crash_investigator_backtrace_unix.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 25
//

#include <crash_investigator/crash_investigator_internal_header.h>
#include <crash_investigator/core/rawallocfree.hpp>
#include <crash_investigator/callback.hpp>
#include <string.h>
#include <execinfo.h>
#include <alloca.h>


namespace crash_investigator {

#define CRASH_INVEST_SYMBOLS_COUNT_MAX  256

struct Backtrace{
    void** ppBuffer;
    int    stackDeepness;
    int    reserved01;
};


CRASH_INVEST_DLL_PRIVATE void FreeBacktraceData(Backtrace* a_data)
{
    if(a_data){
        freen(a_data->ppBuffer);
        freen(a_data);
    }
}


CRASH_INVEST_DLL_PRIVATE Backtrace* InitBacktraceDataForCurrentStack(int a_goBackInTheStackCalc)
{
    Backtrace* pReturn = static_cast<Backtrace*>(mallocn(sizeof(Backtrace)));
    if(!pReturn){return CRASH_INVEST_NULL;}

    const int cnMaxSymbolCount = CRASH_INVEST_SYMBOLS_COUNT_MAX+a_goBackInTheStackCalc;

    pReturn->reserved01 = 0;

    void** ppBuffer = static_cast<void**>(alloca(static_cast<size_t>(cnMaxSymbolCount)*sizeof(void*)));
    int nInitialDeepness = backtrace(ppBuffer,cnMaxSymbolCount);
    if(nInitialDeepness>a_goBackInTheStackCalc){
        pReturn->stackDeepness = nInitialDeepness-a_goBackInTheStackCalc;
        pReturn->ppBuffer = static_cast<void**>(mallocn(static_cast<size_t>(pReturn->stackDeepness)*sizeof(void*)));
        if(!(pReturn->ppBuffer)){FreeBacktraceData(pReturn);return CRASH_INVEST_NULL;}
        memcpy(pReturn->ppBuffer,&(ppBuffer[a_goBackInTheStackCalc]),static_cast<size_t>(pReturn->stackDeepness));
    }
    else{
        pReturn->stackDeepness = nInitialDeepness;
        pReturn->ppBuffer = static_cast<void**>(mallocn(static_cast<size_t>(pReturn->stackDeepness)*sizeof(void*)));
        if(!(pReturn->ppBuffer)){FreeBacktraceData(pReturn);return CRASH_INVEST_NULL;}
        memcpy(pReturn->ppBuffer,ppBuffer,static_cast<size_t>(pReturn->stackDeepness));
    }

    return pReturn;
}


CRASH_INVEST_DLL_PRIVATE void ConvertBacktraceToNames(const Backtrace* a_data, ::std::vector< StackItem>*  a_pStack)
{
    if(a_data){
        char** ppStrings = backtrace_symbols(a_data->ppBuffer,a_data->stackDeepness);
        if(!ppStrings){return;}

        StackItem* pStackItem;
        const size_t cunSynbols(a_data->stackDeepness);
        a_pStack->resize(cunSynbols);

        for(size_t i(0); i < cunSynbols; ++i){
            pStackItem = &(a_pStack->operator [](i));
            pStackItem->address = a_data->ppBuffer[i];
            pStackItem->funcName = ppStrings[i];
        }

        freen(ppStrings);
    }
}



}  // namespace crash_investigator {
