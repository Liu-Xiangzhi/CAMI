#include <stdio.h>

static void f(int*);

void f(int* p)
{
    printf("%d\n", *p = 2);
}

int main()
{
    const int x = 1;
    f((int*)&x);
}
