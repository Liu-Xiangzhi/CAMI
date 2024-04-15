#include <stdio.h>
#include <stdint.h>

int main()
{
    uint32_t x = 0xffffffffU;
    printf("%u\n", -(x + 1));
}
