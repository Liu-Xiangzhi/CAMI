#include <stdio.h>

static void f(int, int*);

void f(int x, int* p)
{
    printf("%d\n", *(p + x - 1));
}

int main()
{
    int x[2] = {0};
    f(2, x);
}
