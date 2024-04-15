#include <stdio.h>
#include <stdint.h>

static void f(int32_t);

void f(int32_t x)
{
    printf("%d\n", x / -1);
}

int main()
{
    int32_t x = INT32_MIN;
    f(x);
}
