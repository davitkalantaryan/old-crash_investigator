//
// file:		main_double_free01_test.cpp
// path:		src/tests/main_double_free01_test.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#ifdef _MSC_VER
#include <crash_investigator/crash_investigator_internal_header.h>
#include <crash_investigator/callback.hpp>
#endif
#include <stdlib.h>
#include <stdio.h>
//#include <crash_investigator/crash_investigator_malloc_free_hook.h>

static void Corruption01();
static void Corruption02();
static void Corruption03();
static void Corruption04();
static void Corruption05();
static void Corruption06();
static void Corruption07();
static void Corruption08();
static void Corruption09();

#ifdef _MSC_VER
#pragma warning (disable:4996)
// crash_investiator_new_malloc.lib;initial_malloc_free_test.lib
#pragma comment(lib,"crash_investiator_new_malloc.lib")
//#pragma comment(lib,"initial_malloc_free.lib")
//#pragma comment(lib,"Dbghelp.lib")
#endif


int main(int a_argc, char* a_argv[])
{
	//printf("If debugging is needed, then connect with debugger, then press enter to proceed %p ! ", g_callers_malloc);
	printf("If debugging is needed, then connect with debugger, then press enter to proceed  ! ");
	fflush(stdout);
	getchar();
	
	if(a_argc<2){
        fprintf(stderr,"ERROR: specify number [1..9] to select hook to test\n");
		return 1;
	}
	
	int nHookToTest = atoi(a_argv[1]);
	switch(nHookToTest){
	case 1:
		Corruption01();
		break;
	case 2:
		Corruption02();
		break;
	case 3:
		Corruption03();
		break;
	case 4:
		Corruption04();
        break;
    case 5:
        Corruption05();
		break;
    case 6:
        Corruption06();
        break;
    case 7:
        Corruption07();
        break;
    case 8:
        Corruption08();
        break;
	case 9:
		Corruption09();
		break;
	default:
		fprintf(stderr,"ERROR: Number of hook should be in the region [1..9]\n");
		return 1;
	}
	
	fflush(stdout);
	return 0;
}


static void Corruption01(){  // double free
	// https://stackoverflow.com/questions/33374483/what-does-visual-studio-do-with-a-deleted-pointer-and-why
	// mor Microsoft compiler, after delete compiler modifies pointer value to 0x8123
	// in order to prevent this, disable SDL check 
	// Properties -> Configuration Properties -> C/C++ -> General -> SDL checks
	int* pnValue = new int;
	delete pnValue;
	delete pnValue;
}
static void Corruption02(){  // bad realloc 01 (wrong address)
    void* pnValue = realloc(reinterpret_cast<void*>(0x100),200);
    free(pnValue);
}
static void Corruption03(){  // bad realloc 02 (deallocated address)
    void* pnValue = malloc(200);
    free(pnValue);
    void* pnValueRe = realloc(pnValue,400);  // we have problem here
    free(pnValueRe);
}
static void Corruption04(){  // bad realloc 03 (allocated by new address)
    int* pnValue = new int;
    void* pnValueRe = realloc(pnValue,400);  // we have problem here
    free(pnValueRe);
}
static void Corruption05(){  // dealloc missmatch 01
    int* pnValue = new int[2];
    delete pnValue;
}
static void Corruption06(){  // dealloc missmatch 02
	int* pnValue = static_cast<int*>(malloc(sizeof(int)));
	delete pnValue;
}
static void Corruption07(){  // dealloc missmatch 03
    int* pnValue = new int;
    free(pnValue);
}
static void Corruption08(){  // indirect double free
	FILE* pFile = fopen("/tmp/aaa.txt","w");
	printf("pFile: %p\n",static_cast<void*>(pFile));
	fflush(stdout);
	if(pFile){
		fclose(pFile);
		fclose(pFile);
	}
}
static void Corruption09() {  // memory leak
	for (int i(0); i < 12; ++i) {
		new int;
	}
}
