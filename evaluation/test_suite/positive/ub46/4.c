#include <stdio.h>
static void f(int, int*);

static void f(int off, int* x)
{
    printf("%d\n", x[off]);
}

int main()
{
    int x[2][2] = {0};
    f(3, (int*)x);
}
