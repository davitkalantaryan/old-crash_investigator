//
// file:		main_double_free01_test.cpp
// path:		src/tests/main_double_free01_test.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#include <stdlib.h>

static void Corruption01();
static void Corruption02();
static void Corruption03();

int main()
{
	Corruption01();
	Corruption02();
	Corruption03();
	
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
