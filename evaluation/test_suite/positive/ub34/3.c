#include <stdio.h>

static void f(int*,int*);
void f(int* p, int* q)
{
    printf("%d\n", (*p)++ + ++(*q));
}

int main()
{
    int i = 1;
    f(&i, &i);
}
