#include <stdio.h>
#include <stdint.h>

static void f(int,int32_t);

void f(int c, int32_t x)
{
    if (c) {
        printf("%d\n", x / -1);
    }
}

int main()
{
    int32_t x = INT32_MIN;
    f(0, x);
}
