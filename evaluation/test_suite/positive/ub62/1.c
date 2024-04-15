#include <stdio.h>

int main()
{
    volatile int x = 1;
    int* p = (int*)&x;
    printf("%d\n", *p);
}
