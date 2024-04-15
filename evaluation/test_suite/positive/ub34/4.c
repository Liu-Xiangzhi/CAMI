#include <stdio.h>

static void f(int,int*,int*,int*);
void f(int x, int* p, int* q, int* r)
{
    int* s = x > 1 ? q : r;
    printf("%d\n", (*p)++ + ++(*s));
}

int main()
{
    int i = 1;
    int j = 2;
    f(10, &i, &i, &j);
}
