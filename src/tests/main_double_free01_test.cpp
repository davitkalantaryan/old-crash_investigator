//
// file:		main_double_free01_test.cpp
// path:		src/tests/main_double_free01_test.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#include <stdlib.h>
#include <stdio.h>

static void Corruption01();
static void Corruption02();
static void Corruption03();
static void Corruption04();
static void Corruption05();
static void Corruption06();
static void Corruption07();
static void Corruption08();

int main(int a_argc, char* a_argv[])
{
	printf("If debugging is needed, then connect with debugger, then press enter to proceed! ");
	fflush(stdout);
	getchar();
	
	if(a_argc<2){
        fprintf(stderr,"ERROR: specify number [1..6] to select hook to test\n");
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
	default:
		fprintf(stderr,"ERROR: Number of hook should be in the region [1..3]\n");
		return 1;
	}
	
	fflush(stdout);
	return 0;
}


static void Corruption01(){  // double free
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
