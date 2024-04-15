#include <stdio.h>
static void f(int*);

static void f(int* x)
{
    printf("%d\n", x[3]);
}

int main()
{
    int x[2][2] = {0};
    f((int*)x);
}
