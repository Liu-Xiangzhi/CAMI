#include <stdio.h>

static void f(int*);

void f(int* p)
{
    printf("%d\n", *p);
}

int main()
{
    volatile int x = 1;
    f((int*)&x);
}
