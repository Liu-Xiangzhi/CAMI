#include <stdint.h>
#include <stdio.h>

int main()
{
    float x = 0x7fff;
    int16_t y = (int16_t)x;
    printf("%d\n", (int)y);
}
