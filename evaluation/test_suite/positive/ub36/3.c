#include <stdio.h>

static void f(void*);

void f(void* p)
{
    printf("%d\n", *(int*)p);
}

int main()
{
    double x = 1.2;
    f(&x);
}
