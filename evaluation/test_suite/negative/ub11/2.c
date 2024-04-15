#include <stdio.h>
static void f(int*);
void f(int* p) 
{
    printf("%d", *p);
}

int main()
{
    int x = 0;
    f(&x);
}
