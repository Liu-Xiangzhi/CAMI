#include <stdio.h>
#include <stdint.h>

int main()
{
    uint32_t x = (uint32_t)(INT32_MIN);
    printf("%u\n", x / (uint32_t)-1);
}
