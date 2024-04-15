#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

int main()
{
    int x = 0;
    uintptr_t y = (uintptr_t)&x;
    int* p = (int*)y;
    printf("%d\n", *p);
}
