#include <stdio.h>

int main()
{
    volatile int x = 1;
    volatile int* p = &x;
    *p = 2;
    printf("%d\n", *p);
}
