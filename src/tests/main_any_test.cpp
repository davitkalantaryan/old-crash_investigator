//
// file:		main_any_test.cpp
// path:		src/tests/main_any_test.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 29
//

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>


int main(int, char* [])
{
	void* pMemory = malloc(10);
	realloc(pMemory,100);
	realloc(pMemory,200);  // this is a potential double free
	return 0;
}
