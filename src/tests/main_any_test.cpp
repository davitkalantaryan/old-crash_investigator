

#include <stdio.h>



int main(int a_argc, char* a_argv[])
{
    char* pVar = new char[100];
    printf("a_argc=%d, a-argv[0]=\"%s\"", a_argc,a_argv[0]);
    printf("pVar = %p\n",(void*)pVar);
    delete [] pVar;

    return 0;
}
