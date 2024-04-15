#include <stdio.h>

static void f(void*);

void f(void* p)
{
    printf("%d\n", *(int*)p);
}

int main()
{
    unsigned int x = 0xffffffffU;
    f(&x);
}
