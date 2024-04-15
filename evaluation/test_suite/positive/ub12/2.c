#include <stdio.h>

int main()
{
    int x = 0;
    int* p = &x;
    *(((char*)p) + 1) = *(char*)&p;
    printf("%p\n", (void*)p);
}
