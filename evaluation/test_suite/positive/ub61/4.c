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
    const int x = 1;
    f(1, (int*)&x);
}
