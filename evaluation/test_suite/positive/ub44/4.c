#include <stdio.h>

static void f(int,int, int*);

void f(int c,int x, int* p)
{
    if (c) {
        printf("%d\n", p[x]);
    }
}

int main()
{
    int x[2] = {0};
    f(1, 2, x);
}
