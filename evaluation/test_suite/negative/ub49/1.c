#include <stdio.h>
#include <stdint.h>

int main()
{
    int32_t x = -1578;
    printf("%d\n", (int)((uint32_t)x << 2));
}