#include <stdio.h>

int main()
{
    volatile int x = 1;
    volatile int* p = &x;
    printf("%d\n", *p);
}
