#include <stdio.h>
#include <stdint.h>

static void f(uint32_t);

void f(uint32_t x)
{
    printf("%d\n", x / (uint32_t)-1);
}

int main()
{
    uint32_t x = (uint32_t)(INT32_MIN);
    f(x);
}
