#include <stdio.h>

static void f(int,int,int);

void f(int c, int x, int y)
{
    printf("%d\n", c ? x / y : y / x);
}

int main()
{
    f(0, 0 , 1);
}
