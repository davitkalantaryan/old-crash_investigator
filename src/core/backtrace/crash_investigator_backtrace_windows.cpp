//
// file:		crash_investigator_backtrace_unix.cpp
// path:		src/core/backtrace/crash_investigator_backtrace_unix.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 25
//

#ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
#if defined(_WIN32) || defined(__INTELLISENSE__)

#include <crash_investigator/crash_investigator_internal_header.h>
#include <crash_investigator/core/rawallocfree.hpp>
#include <crash_investigator/callback.hpp>
#include <malloc.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <DbgHelp.h>


//#include <crash_investigator/core/rawallocfree.hpp>
//#include <crash_investigator/callback.hpp>
//#include <string.h>

// see: https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/bb204633(v=vs.85)?redirectedfrom=MSDN


namespace crash_investigator {



struct Backtrace{
    void** ppBuffer;
    int    stackDeepness;
    int    reserved01;
};


CPPUTILS_DLL_PRIVATE bool IsTheSameStack(const Backtrace* a_stack1, const Backtrace* a_stack2)
{
	return (a_stack1->stackDeepness > 0) && (a_stack1->stackDeepness == a_stack2->stackDeepness) &&
		(memcmp(a_stack1->ppBuffer, a_stack2->ppBuffer, CPPUTILS_STATIC_CAST(size_t, a_stack1->stackDeepness) * sizeof(void*)) == 0);
}


CPPUTILS_DLL_PRIVATE size_t HashOfTheStack(const Backtrace* a_stack)
{
	size_t cunRet(0);
	size_t unMult(1);
	for (int i(0); i < a_stack->stackDeepness; ++i, unMult*=1000) {
		cunRet += ((size_t)a_stack->ppBuffer[i]) * unMult;
	}
	return cunRet;
}


CPPUTILS_DLL_PRIVATE void FreeBacktraceData(Backtrace* a_data)
{
	if (a_data) {
		freen(a_data->ppBuffer);
		freen(a_data);
	}
}


CPPUTILS_DLL_PRIVATE Backtrace* InitBacktraceDataForCurrentStack(int a_goBackInTheStackCalc)
{
	++a_goBackInTheStackCalc;
	void** ppBuffer = static_cast<void**>(alloca(static_cast<size_t>(64) * sizeof(void*)));
	WORD countOfStacks = CaptureStackBackTrace(static_cast<DWORD>(a_goBackInTheStackCalc), static_cast<DWORD>(64 - a_goBackInTheStackCalc), ppBuffer, CPPUTILS_NULL);

	if (countOfStacks < 1) { return CPPUTILS_NULL; }

    Backtrace* pReturn = static_cast<Backtrace*>(mallocn(sizeof(Backtrace)));
    if(!pReturn){return CPPUTILS_NULL;}

	pReturn->stackDeepness = static_cast<int>(countOfStacks);

	pReturn->ppBuffer = static_cast<void**>(mallocn(static_cast<size_t>(pReturn->stackDeepness) * sizeof(void*)));
	if (!(pReturn->ppBuffer)) { FreeBacktraceData(pReturn); return CPPUTILS_NULL; }

	memcpy(pReturn->ppBuffer, ppBuffer, static_cast<size_t>(pReturn->stackDeepness) * sizeof(void*));

	return pReturn;
}

static void GetSymbolInfo(StackItem* a_pItem);

CPPUTILS_DLL_PRIVATE void ConvertBacktraceToNames(const Backtrace* a_data, ::std::vector< StackItem>*  a_pStack)
{
	StackItem* pStackItem;
	const size_t cunSynbols(a_data->stackDeepness);
	a_pStack->resize(cunSynbols);

	for (size_t i(0); i < cunSynbols; ++i) {
		pStackItem = &(a_pStack->operator [](i));
		pStackItem->reserved01 = 0;
		pStackItem->address = a_data->ppBuffer[i];
		GetSymbolInfo(pStackItem);
	}
}

static HANDLE s_currentProcess = CPPUTILS_NULL;
class SymIniter {
public:
	SymIniter() {
		s_currentProcess = GetCurrentProcess();
		if (!SymInitialize(s_currentProcess, CPPUTILS_NULL, TRUE)){
			// SymInitialize failed
			DWORD error = GetLastError();
			fprintf(stderr,"SymInitialize returned error : %d\n", static_cast<int>(error));
			return;
		}
	}
}static s_SymIniter;


static void GetSymbolInfo(StackItem* a_pStackItem)
{
	// https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-symbol-information-by-address
	const DWORD64  dwAddress = static_cast<DWORD64>(reinterpret_cast<size_t>(a_pStackItem->address));

	{
		DWORD64  dwDisplacement = 0;
		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		if (SymFromAddr(s_currentProcess, dwAddress, &dwDisplacement, pSymbol)) {
			a_pStackItem->funcName = pSymbol->Name;
		}
		else {
			// SymFromAddr failed
			//DWORD error = GetLastError();
			//fprintf(stderr, "SymFromAddr returned error : %d\n", static_cast<int>(error));
		}
	}

	
	
	{
		DWORD  dwDisplacement;
		IMAGEHLP_LINE64 line;

		SymSetOptions(SYMOPT_LOAD_LINES);

		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

		if (SymGetLineFromAddr64(s_currentProcess, dwAddress, &dwDisplacement, &line)){
			if (line.FileName) {
				a_pStackItem->sourceFileName = line.FileName;
			}
			a_pStackItem->line = static_cast<int>(line.LineNumber);
		}
		else{
			// SymGetLineFromAddr64 failed
			a_pStackItem->line = -1;
			//DWORD error = GetLastError();
			//fprintf(stderr,"SymGetLineFromAddr64 returned error : %d\n", static_cast<int>(error));
		}
	}

}



}  // namespace crash_investigator {


#endif  // #indef _WIN32
#endif // #ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
