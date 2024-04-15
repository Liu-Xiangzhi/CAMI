#include <stdint.h>
#include <stdio.h>
int main()
{
    int x = 0;
    int* p = &x;
    uintptr_t y = (uintptr_t)p;
    printf("%d\n", (int)y);
}
