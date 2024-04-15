#include <stdio.h>
#include <stdint.h>

static void f(int, int, int);

void f(int c, int x, int y)
{
    if (c) {
        printf("%d\n", x << y);
    }
}

int main()
{
    f(1, -1, 5);
}
