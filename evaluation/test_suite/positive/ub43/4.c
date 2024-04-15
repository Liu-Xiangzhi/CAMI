#include <stdio.h>

static void f(int*,int);

void f(int* p, int x)
{
    printf("%p\n", (void*)(p + x));
}

int main()
{
    int x[2] = {0};
    f(x + 1, 2);
}
