#include <stdio.h>

static void f(int, int*);

void f(int c, int* p)
{
    if (c) {
        printf("%d\n", *p = 2);
    }
}

int main()
{
    volatile int x = 1;
    f(0, (int*)&x);
}
