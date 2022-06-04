//
// file:		crash_investigator_backtrace_unix.cpp
// path:		src/core/backtrace/crash_investigator_backtrace_unix.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 25
//

#ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
#if !defined(_WIN32) || defined(__INTELLISENSE__)

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

CPPUTILS_DLL_PRIVATE bool IsTheSameStack(const Backtrace* a_stack1, const Backtrace* a_stack2)
{
	return (a_stack1->stackDeepness > 0) && (a_stack1->stackDeepness == a_stack2->stackDeepness) &&
		(memcmp(a_stack1->ppBuffer, a_stack2->ppBuffer, CPPUTILS_STATIC_CAST(size_t, a_stack1->stackDeepness)*sizeof(void*)) == 0);
}


CPPUTILS_DLL_PRIVATE size_t HashOfTheStack(const Backtrace* a_stack)
{
	size_t cunRet(0);
	size_t unMult(1);
	for (int i(0); i < a_stack->stackDeepness; ++i, unMult *= 1000) {
		cunRet += ((size_t)a_stack->ppBuffer[i]) * unMult;
	}
	return cunRet;
}


CPPUTILS_DLL_PRIVATE void FreeBacktraceData(Backtrace* a_data)
{
    if(a_data){
        freen(a_data->ppBuffer);
        freen(a_data);
    }
}


CPPUTILS_DLL_PRIVATE Backtrace* InitBacktraceDataForCurrentStack(int a_goBackInTheStackCalc)
{
    Backtrace* pReturn = static_cast<Backtrace*>(mallocn(sizeof(Backtrace)));
    if(!pReturn){return CPPUTILS_NULL;}

    const int cnMaxSymbolCount = CRASH_INVEST_SYMBOLS_COUNT_MAX+a_goBackInTheStackCalc;

    pReturn->reserved01 = 0;

    void** ppBuffer = static_cast<void**>(alloca(static_cast<size_t>(cnMaxSymbolCount)*sizeof(void*)));
    int nInitialDeepness = backtrace(ppBuffer,cnMaxSymbolCount);
    if(nInitialDeepness>a_goBackInTheStackCalc){
        pReturn->stackDeepness = nInitialDeepness-a_goBackInTheStackCalc;
        pReturn->ppBuffer = static_cast<void**>(mallocn(static_cast<size_t>(pReturn->stackDeepness)*sizeof(void*)));
        if(!(pReturn->ppBuffer)){FreeBacktraceData(pReturn);return CPPUTILS_NULL;}
		memcpy(pReturn->ppBuffer,&(ppBuffer[a_goBackInTheStackCalc]),static_cast<size_t>(pReturn->stackDeepness)*sizeof(void*));
    }
    else{
        pReturn->stackDeepness = nInitialDeepness;
        pReturn->ppBuffer = static_cast<void**>(mallocn(static_cast<size_t>(pReturn->stackDeepness)*sizeof(void*)));
        if(!(pReturn->ppBuffer)){FreeBacktraceData(pReturn);return CPPUTILS_NULL;}
		memcpy(pReturn->ppBuffer,ppBuffer,static_cast<size_t>(pReturn->stackDeepness)*sizeof(void*));
    }

    return pReturn;
}


CPPUTILS_DLL_PRIVATE void ConvertBacktraceToNames(const Backtrace* a_data, ::std::vector< StackItem>*  a_pStack)
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
            pStackItem->dllName = ppStrings[i];
			pStackItem->reserved01 = 0;
			pStackItem->line = -1;
        }

        freen(ppStrings);
    }
}



}  // namespace crash_investigator {


/*//// dwarf test*/

//#ifdef CRASH_INVEST_TRANSLATE_ADDRESSES_TO_LINES
#if 1

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <err.h>

static char unknown[] = { '?', '?', '\0' };
static Dwarf_Addr section_base;

//static
void
translate(Dwarf_Debug dbg, Elf *e, const char* addrstr)
{
	Dwarf_Die die, ret_die;
	Dwarf_Line *lbuf;
	Dwarf_Error de;
	Dwarf_Half tag;
	Dwarf_Unsigned lopc, hipc, addr, lineno, plineno;
	Dwarf_Signed lcount;
	Dwarf_Addr lineaddr, plineaddr;
	Dwarf_Off off;
	struct CU *cu;
	struct Func *f;
	const char *funcname;
	char *file, *file0, *pfile;
	char demangled[1024];
	int ec, i, ret;

	addr = strtoull(addrstr, NULL, 16);
	addr += section_base;
	lineno = 0;
	file = unknown;
	cu = NULL;
	die = NULL;

	while ((ret = dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL, NULL,
	    &de)) ==  DW_DLV_OK) {
		die = NULL;
		while (dwarf_siblingof(dbg, die, &ret_die, &de) == DW_DLV_OK) {
			if (die != NULL)
				dwarf_dealloc(dbg, die, DW_DLA_DIE);
			die = ret_die;
			if (dwarf_tag(die, &tag, &de) != DW_DLV_OK) {
				warnx("dwarf_tag failed: %s",
				    dwarf_errmsg(de));
				goto next_cu;
			}

			/* XXX: What about DW_TAG_partial_unit? */
			if (tag == DW_TAG_compile_unit)
				break;
		}
		if (ret_die == NULL) {
			warnx("could not find DW_TAG_compile_unit die");
			goto next_cu;
		}
		if (dwarf_attrval_unsigned(die, DW_AT_low_pc, &lopc, &de) ==
		    DW_DLV_OK) {
			if (dwarf_attrval_unsigned(die, DW_AT_high_pc, &hipc,
			   &de) == DW_DLV_OK) {
				/*
				 * Check if the address falls into the PC
				 * range of this CU.
				 */
				if (handle_high_pc(die, lopc, &hipc) !=
				    DW_DLV_OK)
					goto out;
			} else {
				/* Assume ~0ULL if DW_AT_high_pc not present */
				hipc = ~0ULL;
			}

			/*
			 * Record the CU in the hash table for faster lookup
			 * later.
			 */
			if (dwarf_dieoffset(die, &off, &de) != DW_DLV_OK) {
				warnx("dwarf_dieoffset failed: %s",
				    dwarf_errmsg(de));
				goto out;
			}
			HASH_FIND(hh, culist, &off, sizeof(off), cu);
			if (cu == NULL) {
				if ((cu = calloc(1, sizeof(*cu))) == NULL)
					err(EXIT_FAILURE, "calloc");
				cu->off = off;
				cu->lopc = lopc;
				cu->hipc = hipc;
				STAILQ_INIT(&cu->funclist);
				HASH_ADD(hh, culist, off, sizeof(off), cu);
			}

			if (addr >= lopc && addr < hipc)
				break;
		}

	next_cu:
		if (die != NULL) {
			dwarf_dealloc(dbg, die, DW_DLA_DIE);
			die = NULL;
		}
	}

	if (ret != DW_DLV_OK || die == NULL)
		goto out;

	switch (dwarf_srclines(die, &lbuf, &lcount, &de)) {
	case DW_DLV_OK:
		break;
	case DW_DLV_NO_ENTRY:
		/* If a CU lacks debug info, just skip it. */
		goto out;
	default:
		warnx("dwarf_srclines: %s", dwarf_errmsg(de));
		goto out;
	}

	plineaddr = ~0ULL;
	plineno = 0;
	pfile = unknown;
	for (i = 0; i < lcount; i++) {
		if (dwarf_lineaddr(lbuf[i], &lineaddr, &de)) {
			warnx("dwarf_lineaddr: %s", dwarf_errmsg(de));
			goto out;
		}
		if (dwarf_lineno(lbuf[i], &lineno, &de)) {
			warnx("dwarf_lineno: %s", dwarf_errmsg(de));
			goto out;
		}
		if (dwarf_linesrc(lbuf[i], &file0, &de)) {
			warnx("dwarf_linesrc: %s", dwarf_errmsg(de));
		} else
			file = file0;
		if (addr == lineaddr)
			goto out;
		else if (addr < lineaddr && addr > plineaddr) {
			lineno = plineno;
			file = pfile;
			goto out;
		}
		plineaddr = lineaddr;
		plineno = lineno;
		pfile = file;
	}

out:
	f = NULL;
	funcname = NULL;
	if (ret == DW_DLV_OK && (func || inlines) && cu != NULL) {
		if (cu->srcfiles == NULL)
			if (dwarf_srcfiles(die, &cu->srcfiles, &cu->nsrcfiles,
			    &de))
				warnx("dwarf_srcfiles: %s", dwarf_errmsg(de));
		if (STAILQ_EMPTY(&cu->funclist)) {
			collect_func(dbg, die, NULL, cu);
			die = NULL;
		}
		f = search_func(cu, addr);
		if (f != NULL)
			funcname = f->name;
	}

	if (print_addr) {
		if ((ec = gelf_getclass(e)) == ELFCLASSNONE) {
			warnx("gelf_getclass failed: %s", elf_errmsg(-1));
			ec = ELFCLASS64;
		}
		if (ec == ELFCLASS32) {
			if (pretty_print)
				printf("0x%08jx: ", (uintmax_t) addr);
			else
				printf("0x%08jx\n", (uintmax_t) addr);
		} else {
			if (pretty_print)
				printf("0x%016jx: ", (uintmax_t) addr);
			else
				printf("0x%016jx\n", (uintmax_t) addr);
		}
	}

	if (func) {
		if (funcname == NULL)
			funcname = unknown;
		if (demangle && !elftc_demangle(funcname, demangled,
		    sizeof(demangled), 0)) {
			if (pretty_print)
				printf("%s at ", demangled);
			else
				printf("%s\n", demangled);
		} else {
			if (pretty_print)
				printf("%s at ", funcname);
			else
				printf("%s\n", funcname);
		}
	}

	(void) printf("%s:%ju\n", base ? basename(file) : file,
	    (uintmax_t) lineno);

	if (ret == DW_DLV_OK && inlines && cu != NULL &&
	    cu->srcfiles != NULL && f != NULL && f->inlined_caller != NULL)
		print_inlines(cu, f->inlined_caller, f->call_file,
		    f->call_line);

	if (die != NULL)
		dwarf_dealloc(dbg, die, DW_DLA_DIE);

	/*
	 * Reset internal CU pointer, so we will start from the first CU
	 * next round.
	 */
	while (ret != DW_DLV_NO_ENTRY) {
		if (ret == DW_DLV_ERROR)
			errx(EXIT_FAILURE, "dwarf_next_cu_header: %s",
			    dwarf_errmsg(de));
		ret = dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL, NULL,
		    &de);
	}
}

#endif // #ifdef CRASH_INVEST_TRANSLATE_ADDRESSES_TO_LINES


#endif  // #ifndef _WIN32
#endif // #ifndef CRASH_INVEST_DO_NOT_USE_AT_ALL
