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

int main(int a_argc, char* a_argv[])
{
	printf("If debugging is needed, then connect with debugger, then press enter to proceed! ");
	fflush(stdout);
	getchar();
	
	if(a_argc<2){
		fprintf(stderr,"ERROR: specify number [1..3] to select hook to test\n");
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
	default:
		fprintf(stderr,"ERROR: Number of hook should be in the region [1..3]\n");
		return 1;
	}
	
	fflush(stdout);
	return 0;
}


static void Corruption01()
{
	int* pnValue = new int;
	delete pnValue;
	delete pnValue;
}


static void Corruption02()
{
	int* pnValue = new int[2];
	delete pnValue;
}


static void Corruption03()
{
	int* pnValue = static_cast<int*>(malloc(sizeof(int)));
	free(pnValue);
	delete pnValue;
}

static void Corruption04()
{
	FILE* pFile = fopen("/tmp/aaa.txt","w");
	printf("pFile: %p\n",static_cast<void*>(pFile));
	fflush(stdout);
	if(pFile){
		fclose(pFile);
		fclose(pFile);
	}
}


static void Corruption05()
{
    int* pnValue = new int;
    free(pnValue);
}
