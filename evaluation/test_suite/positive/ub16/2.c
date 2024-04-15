#include <stdint.h>
#include <stdio.h>

int main()
{
    int x = 0;
    int* p = &x;
    int16_t y = (int16_t)p;
    printf("%d\n", (int)y);
}
