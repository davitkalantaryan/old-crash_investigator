//
// file:		main_double_free01_test.cpp
// path:		src/tests/main_double_free01_test.cpp
// created by:	Davit Kalantaryan (davit.kalataryan@desy.de)
// created on:	2021 Nov 19
//

#include <stdio.h>

int main()
{
	printf("If debugging is needed, then connect with debugger, then press enter to proceed  ! ");
	fflush(stdout);
	getchar();
	
	for(int i(0); i<1000;++i){
		int* pI = new int;
		printf("%.4d  pI=%p\n",i,static_cast<void*>(pI));
	}
	
	return 0;
}
