#include <stdio.h>
static void f(int*);

static void f(int* x)
{
    printf("%d\n", (*(int (*)[2])&x[2])[1]);
}

int main()
{
    int x[2][2] = {0};
    f((int*)x);
}
